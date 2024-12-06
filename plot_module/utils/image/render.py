import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import os

PATH = './output/'

for filename in os.listdir(PATH):
    if not filename.endswith('.txt'):
        continue
    file_path = os.path.join(PATH, filename)
    data = np.loadtxt(file_path, delimiter=',')

    # The data should be reshaped based on the dimensions used in the C++ code
    height = 1000
    width = int(data.size / height)
    data = data.reshape((height, width))

    print("Generating image for "+filename.split('_')[0]+"...")

    # Plotting the data
    plt.figure(figsize=(width / 100, height / 100), dpi=100)
    plt.imshow(data, cmap=cm.plasma, extent=[-2, 1, -1, 1])
    plt.colorbar()
    plt.title(filename.split('_')[0].capitalize()+" Mandelbrot Set")

    print("Saving image "+filename.split('_')[0]+"_mandelbrot_set.png")

    # Save the image to a file
    output_image_path = PATH+"rendered/"+filename.split('_')[0]+'_mandelbrot_set.png'
    plt.savefig(output_image_path, bbox_inches='tight')

print("Done!")

