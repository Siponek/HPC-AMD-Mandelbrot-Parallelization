'''
- Take the file sequential_out.txt as reference
- For each txt file in output folder, compute the accuracy

- Accuracy is computed as the sum of the absolute difference between the values of the two files
'''

import os
import numpy as np
from alive_progress import alive_bar

PATH = './output/'

def compute_accuracy(reference_file, output_file):
    ref = np.loadtxt(reference_file, delimiter=',')
    out = np.loadtxt(output_file, delimiter=',')

    differences = np.abs(ref - out)

    return np.sum(differences), np.sum(differences > 0), ref.shape[0] * ref.shape[1]

def main():
    reference_file = PATH + 'sequential_out.txt'
    print(f'Reference file: {reference_file}')

    accurancies = []

    # search for all files in output folder and subfolders
    all_files = []
    for root, _, files in os.walk(PATH):
        for file in files:
            if file.endswith('.txt') and file != 'sequential_out.txt':
                all_files.append(os.path.join(root, file))

    # compute accuracy for each file
    with alive_bar(len(all_files)) as bar:
        for file in all_files:
            tmp = {}
            tmp['file'] = file
            accuracy, count, total = compute_accuracy(reference_file, file)
            tmp['accuracy'] = accuracy
            tmp['count'] = count
            tmp['total'] = total
            accurancies.append(tmp)
            bar()

    # print results
    print('\nAccuracy:')
    for acc in accurancies:
        print(f'{acc["file"]:60}  {acc["accuracy"]:12} ({acc["count"]:5}/{acc["total"]}) -> {acc["count"]/acc["total"]:.2%}')
    print()

if __name__ == '__main__':
    main()