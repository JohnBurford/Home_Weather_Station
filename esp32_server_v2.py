import os
import sqlite3
from flask import Flask, request, jsonify, render_template
from datetime import datetime, timedelta  # To handle timestamps and date calculations
import pytz
import math

app = Flask(__name__)

# Path to the SQLite database file
db_path = 'weather_database.db'

# Endpoint to handle incoming POST requests with sensor data
@app.route('/api/data', methods=['POST'])
def add_data():
    # Ensure the incoming request is in JSON format
    if request.is_json:
        # Parse JSON payload
        data = request.get_json()

        # Extract sensor data from JSON
        temperature = data.get('temperature')
        humidity = data.get('humidity')
        pressure = data.get('pressure')

        # calculate dewpoint
        if temperature is not None and humidity is not None:
            # Convert temperature from F to C
            temp_c = (temperature - 32) / 1.8

            a = 17.27
            b = 237.7
            alpha = ((a * temp_c) / (b + temp_c)) + math.log(humidity / 100.0)
            dew_point_c = (b * alpha) / (a - alpha)

            # Convert dewpoint back to Fahrenheit
            dew_point = dew_point_c * 1.8 + 32
        else:
            dew_point = None


        # Check if any of the required fields are missing
        if None in [temperature, humidity, pressure]:
            return jsonify({"error": "Missing data"}), 400

        # Get the current timestamp (in ISO 8601 format)
        est_tz = pytz.timezone('US/Eastern')
        timestamp = datetime.now(est_tz)
        #timestamp = rawts.strftime("%d%b%Y %I:%M%p")
        
        # Insert data into the database
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        cursor.execute('''
            INSERT INTO weather (temperature, humidity, pressure, dewpoint, timestamp)
            VALUES (?, ?, ?, ?, ?)
        ''', (temperature, humidity, pressure, dew_point, timestamp))

        conn.commit()  # Commit the transaction
        #Purge old data if needed 
        cursor.execute(f"SELECT COUNT(*) FROM weather")
        row_count = cursor.fetchone()[0]
        if row_count > 2000000:
            rows_to_delete = row_count - 2000000
            print(f" Database bigger than 2M lines, data deleted")
            cursor.execute(f"""
                DELETE FROM weather	
                WHERE timestamp IN (
                    SELECT timestamp
                    FROM weather
                    ORDER BY timestamp ASC
                    LIMIT ?
                )
            """, (rows_to_delete,))
        conn.commit()
        conn.close()   # Close the connection

        # Return a success response

        return jsonify({"message": "Data added successfully"}), 201
    else:
        return jsonify({"error": "Request must be JSON"}), 400

# Endpoint to retrieve all weather data
@app.route('/api/data', methods=['GET'])
def get_data():
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # Fetch all records from the weather table
    cursor.execute('SELECT * FROM weather')
    rows = cursor.fetchall()
 
    conn.close()  # Close the connection
    
    # Format the data into a list of dictionaries
    weather_data = []
    for row in rows:
        weather_data.append({
            "id": row[0],
            "temperature": row[1],
            "humidity": row[2],
            "pressure": row[3],
            "timestamp": row[4],
            "dewpoint": row[5]
        })
    
    #sort data by timestamp
    weather_data.sort(key=lambda x: x["timestamp"])
    #filter data within 2 minutes of each other
    filtered_data = []
    last_timestamp = None
    for entry in weather_data:
        current_timestamp = datetime.fromisoformat(entry["timestamp"])
        if last_timestamp is None or (current_timestamp - last_timestamp) > timedelta(minutes=2):
            filtered_data.append(entry)
            last_timestamp = current_timestamp
    # Return the data as JSON
    return jsonify(filtered_data)

# Endpoint to render the data on a webpage
@app.route('/weather')
def show_weather():
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    # Calculate the timestamp for 72 hours ago
    est_tz = pytz.timezone('US/Eastern')
    last_24_hours = datetime.now(est_tz) - timedelta(days=3)
    last_24_hours_str = last_24_hours.strftime('%Y-%m-%d %H:%M:%S')

    # Fetch weather data from the last 24 hours
    cursor.execute('''
        SELECT temperature, humidity, pressure, timestamp, dewpoint
        FROM weather 
        WHERE timestamp >= ?
    ''', (last_24_hours_str,))
    rows = cursor.fetchall()
    
    conn.close()

    # Prepare data for the graph
    times = []
    temperatures = []
    humidities = []
    pressures = []
    dewpoints = []
    for row in rows:
        times.append(row[3])  # Timestamp
        temperatures.append(row[0])  # Temperature
        humidities.append(row[1])  # Humidity
        pressures.append(row[2])  # Pressure
        dewpoints.append(row[4]) # dewpoint

    # Render the webpage with the data and the graph
    return render_template('weather.html', 
                           times=times, temperatures=temperatures, 
                           humidities=humidities, dewpoints=dewpoints, pressures=pressures)

# Start the Flask application
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
