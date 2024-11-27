import random
import time
import firebase_admin
from firebase_admin import credentials, initialize_app, db
import logging
from datetime import datetime

# Configure logging
logging.basicConfig(level=logging.INFO)

try:
    cred = credentials.Certificate("../firebase_config.json")
    app = initialize_app(cred, {
        'databaseURL': 'https://nuevoiot-e7f97-default-rtdb.firebaseio.com/'
    })
except Exception as e:
    logging.error(f"Error initializing Firebase app: {e}")
    raise

def generate_random_data():
    return {
        'value1': random.randint(10, 100),
        'value2': datetime.now().isoformat(),
    }

while True:
    try:
        data = generate_random_data()
        ref = db.reference('chart_data')
        ref.push(data)
        logging.info(f"Data pushed: {data}")
    except Exception as e:
        logging.error(f"Error pushing data to Firebase: {e}")
    time.sleep(2)  # Simulate data updates every 2 seconds