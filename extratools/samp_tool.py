import wave

def wav_to_c_array(file_path):
    with wave.open(file_path, 'rb') as wav_file:
        num_channels = wav_file.getnchannels()
        sample_width = wav_file.getsampwidth()
        frame_rate = wav_file.getframerate()
        num_frames = wav_file.getnframes()
        raw_frames = wav_file.readframes(num_frames)
        data = bytearray(raw_frames)
        print(len(data))
        c_array_str = "{"
        for i in range(0, len(data)):
            c_array_str += f"{data[i]-128}, "
        c_array_str = c_array_str[:-2]
        c_array_str += "};"
        
        return c_array_str

file_path = "1212.wav"
c_array = wav_to_c_array(file_path)
print(c_array)
