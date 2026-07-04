import json
import os
import sys
from tkinter import Tk, filedialog, messagebox, Button, Label, Frame

# --- CONFIGURACIÓN MECATRÓNICA ---
FACTOR_ESCALA = 64  # Multiplicador ultrasónico para resonancia inductiva

def midi_note_to_freq(midi_number):
    """Convierte nota MIDI a frecuencia real escalada en Hz."""
    if midi_number <= 0:
        return 0
    freq_estandar = 440.0 * (2.0 ** ((midi_number - 69) / 12.0))
    return int(freq_estandar * FACTOR_ESCALA)

def convertir_json_a_csv(input_json, output_csv):
    """Lógica core ultra-robusta tolerante a la falta de metadatos o duraciones."""
    try:
        with open(input_json, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # Filtramos únicamente tracks que tengan la lista de notas y que no esté vacía
        tracks_con_notas = [t for t in data.get("tracks", []) if t.get("notes") and len(t.get("notes")) > 0]
        
        if not tracks_con_notas:
            return False, "No se encontraron pistas con notas musicales en el archivo JSON."
            
        print(f"Se encontraron {len(tracks_con_notas)} pistas válidas con música.")
        
        pista_melodia = tracks_con_notas[0]["notes"]
        
        if len(tracks_con_notas) > 1:
            pista_bajo = tracks_con_notas[1]["notes"]
            print("Asignando Track 0 al Motor 1 y Track 1 al Motor 2.")
        else:
            pista_bajo = pista_melodia
            print("Archivo de una sola pista detectado. Duplicando para ambos motores.")

        eventos_lineales = {}
        
        # Procesar pista 1 (Motor 1)
        for nota in pista_melodia:
            t_inicio = round(nota.get("time", 0.0), 3)
            # Si no tiene 'duration', le ponemos 0.25 segundos (una corchea estándar por defecto)
            duracion_nota = nota.get("duration", 0.25) 
            midi_num = nota.get("midi", 0)
            
            if t_inicio not in eventos_lineales:
                eventos_lineales[t_inicio] = {"m1": 0, "m2": 0, "duracion": duracion_nota}
            eventos_lineales[t_inicio]["m1"] = midi_note_to_freq(midi_num)

        # Procesar pista 2 (Motor 2)
        for nota in pista_bajo:
            t_inicio = round(nota.get("time", 0.0), 3)
            duracion_nota = nota.get("duration", 0.25)
            midi_num = nota.get("midi", 0)
            
            if t_inicio not in eventos_lineales:
                eventos_lineales[t_inicio] = {"m1": 0, "m2": 0, "duracion": duracion_nota}
            eventos_lineales[t_inicio]["m2"] = midi_note_to_freq(midi_num)

        tiempos_ordenados = sorted(eventos_lineales.keys())
        
        with open(output_csv, 'w', encoding='utf-8') as out:
            out.write("# Generado automaticamente por convertidor unificado v3 (KeyError fix)\n")
            out.write("# Motor1,Motor2,DuracionMs\n")
            
            for i in range(len(tiempos_ordenados)):
                t_actual = tiempos_ordenados[i]
                evento = eventos_lineales[t_actual]
                
                if i < len(tiempos_ordenados) - 1:
                    duracion_ms = int((tiempos_ordenados[i+1] - t_actual) * 1000)
                else:
                    duracion_ms = int(evento["duracion"] * 1000)
                    
                if duracion_ms <= 0:
                    duracion_ms = int(evento["duracion"] * 1000)
                    
                out.write(f"{evento['m1']},{evento['m2']},{duracion_ms}\n")
                
        return True, f"¡Éxito! Archivo guardado correctamente en:\n{output_csv}"
    except Exception as e:
        return False, f"Error inesperado al procesar: {str(e)}"
# ==========================================
#  MODO 1: INTERFAZ GRÁFICA (GUI)
# ==========================================
class ConvertidorGUI:
    def __init__(self, ventana):
        self.ventana = ventana
        self.ventana.title("MIDI JSON a CSV Converter 🎼")
        self.ventana.geometry("500x220")
        self.ventana.resizable(False, False)
        
        # Configuración de diseño básica (Modo Claro/Limpio)
        self.lbl_titulo = Label(ventana, text="Convertidor JSON para Caja Musical", font=("Arial", 14, "bold"))
        self.lbl_titulo.pack(pady=15)
        
        self.lbl_info = Label(ventana, text="Selecciona un archivo JSON exportado de MIDI para convertirlo a CSV.", font=("Arial", 10), fg="gray")
        self.lbl_info.pack(pady=5)
        
        self.frame_botones = Frame(ventana)
        self.frame_botones.pack(pady=20)
        
        self.btn_seleccionar = Button(self.frame_botones, text="Buscar archivo JSON ", font=("Arial", 11), command=self.seleccionar_archivo, bg="#E1E1E1", padx=10, pady=5)
        self.btn_seleccionar.pack()

    def seleccionar_archivo(self):
        # Abre el explorador de archivos nativo de Linux
        ruta_json = filedialog.askopenfilename(
            title="Seleccionar JSON de MIDI",
            filetypes=[("Archivos JSON / Texto", "*.json *.txt"), ("Todos los archivos", "*.*")]
        )
        
        if ruta_json:
            # Propone automáticamente el mismo nombre pero con extensión .csv
            ruta_sugerida = os.path.splitext(ruta_json)[0] + ".csv"
            ruta_csv = filedialog.asksaveasfilename(
                title="Guardar archivo CSV resultante",
                initialfile=os.path.basename(ruta_sugerida),
                filetypes=[("Archivos CSV", "*.csv")]
            )
            
            if ruta_csv:
                exito, mensaje = convertir_json_a_csv(ruta_json, ruta_csv)
                if exito:
                    messagebox.showinfo("Procesamiento Completo", mensaje)
                else:
                    messagebox.showerror("Error", mensaje)

# ==========================================
# CONTROLADOR PRINCIPAL DEL SISTEMA
# ==========================================
if __name__ == "__main__":
    # Si el usuario pasa argumentos por terminal (ej: python3 script.py entrada.json salida.csv)
    if len(sys.argv) > 1:
        if len(sys.argv) < 3:
            print("\nModo de uso CLI incorrecto.")
            print("Sintaxis: python3 midi_converter_pro.py <archivo_entrada.json> <archivo_salida.csv>")
            sys.exit(1)
        
        archivo_in = sys.argv[1]
        archivo_out = sys.argv[2]
        
        print(f"\n [Modo CLI] Procesando '{archivo_in}'...")
        exito, msg = convertir_json_a_csv(archivo_in, archivo_out)
        
        if exito:
            print(f"{msg}\n")
        else:
            print(f"{msg}\n")
            sys.exit(1)
            
    # Si no hay argumentos en la terminal, se inicia automáticamente la Interfaz Gráfica
    else:
        root = Tk()
        app = ConvertidorGUI(root)
        root.mainloop()