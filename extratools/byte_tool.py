def file_to_c_uint8_array(input_file, output_file, array_name):
    with open(input_file, 'rb') as f:
        data = f.read()

    with open(output_file, 'w') as f:
        f.write("#include <stdint.h>\n\n")
        f.write("const uint8_t {}[] = {{\n".format(array_name))

        for i, byte in enumerate(data):
            if i != 0:
                f.write(", ")
            f.write("0x{:02x}".format(byte))

        f.write("\n};\n")

if __name__ == "__main__":
    input_file = "space_debris.mod"
    output_file = "space_debris.h"
    array_name = "tracker_data"
    file_to_c_uint8_array(input_file, output_file, array_name)
