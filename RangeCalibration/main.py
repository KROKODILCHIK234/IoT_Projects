import serial
import time
import numpy as np
import matplotlib.pyplot as plt

def check_sufficiency(distances, min_dist, max_dist):
    if len(distances) < 2:
        return False
        
    distances_sorted = np.sort(distances)
    
    coverage = distances_sorted[-1] - distances_sorted[0]
    target_coverage = 0.9 * (max_dist - min_dist)
    
    if coverage < target_coverage:
        return False
        
    gaps = np.diff(distances_sorted)
    if np.max(gaps) > 2.0:
        return False
        
    return True

def collect_data(port, baudrate, min_dist, max_dist):
    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        time.sleep(2) 
    except Exception as e:
        print(f"Ошибка подключения к порту {port}: {e}")
        return None, None

    print(f"--- НАЧАЛО КАЛИБРОВКИ ---")
    print(f"Ожидаемый диапазон: от {min_dist} до {max_dist} см")
    
    ser.write(b"START\n")
    
    analog_data = []
    distance_data = []
    
    print("Двигайте объект перед датчиками...")

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            
            if "," in line:
                try:
                    dist_str, analog_str = line.split(',')
                    dist = float(dist_str)
                    analog = int(analog_str)
                    
                    if min_dist <= dist <= max_dist:
                        distance_data.append(dist)
                        analog_data.append(analog)
                        print(f"Принято: {dist} см | АЦП: {analog}  (Всего валидных точек: {len(distance_data)})")
                        
                        if check_sufficiency(distance_data, min_dist, max_dist):
                            print("\n[УСПЕХ] Собранных данных достаточно!")
                            print("Отправка команды STOP (на Arduino должен загореться светодиод).")
                            ser.write(b"STOP\n")
                            break 
                except ValueError:
                    pass
                    
    ser.close()
    return np.array(analog_data), np.array(distance_data)

def build_models(X_analog, Y_distance, degrees):
    models = {}
    for deg in degrees:
        coefs = np.polyfit(X_analog, Y_distance, deg)
        models[deg] = np.poly1d(coefs)
    return models

def predict_distance(analog_val, model):
    return model(analog_val)

def plot_results(x_data, y_data, models):
    plt.figure(figsize=(10, 6))
    
    plt.scatter(x_data, y_data, color='red', label='Собранные данные', alpha=0.5, marker='o')
    
    x_smooth = np.linspace(min(x_data), max(x_data), 200)
    colors = ['blue', 'green', 'purple']
    
    for idx, (deg, model) in enumerate(models.items()):
        y_predict = predict_distance(x_smooth, model)
        plt.plot(x_smooth, y_predict, color=colors[idx % len(colors)], label=f'Модель {deg}-й степени')
        
    plt.title('Калибровка ИК-дальномера')
    plt.xlabel('Значение АЦП (analogRead)')
    plt.ylabel('Расстояние, см')
    plt.legend()
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    COM_PORT = "COM3"  
    BAUD_RATE = 9600
    
    MIN_CM = 10.0
    MAX_CM = 20.0
    
    analog_vals, dist_vals = collect_data(COM_PORT, BAUD_RATE, MIN_CM, MAX_CM)
    
    if analog_vals is not None and len(analog_vals) > 0:
        print("Построение моделей...")
        test_degrees = [1, 2, 3]
        models_dict = build_models(analog_vals, dist_vals, test_degrees)
        
        print("Отрисовка графика. Закройте окно графика для завершения скрипта.")
        plot_results(analog_vals, dist_vals, models_dict)
        
        test_analog = analog_vals[0]
        print(f"\nПример перевода: при значении АЦП {test_analog}:")
        for deg, model in models_dict.items():
            calc_dist = predict_distance(test_analog, model)
            print(f" - Модель {deg} ст. предсказывает {calc_dist:.2f} см")
    else:
        print("Данные не были собраны. Проверьте подключение Arduino.")
