import os

sep = os.sep

# toggles
dataset = 'power' # [covtype, electricity, mixed, phishing, power]
use_threadpool = False # True

# environments
input_prefix = f'data{sep}{dataset}_orig'
output_prefix = f'data{sep}{dataset}'
sep_field = b','
sep_subfield = b';'

lst_files = os.listdir(input_prefix)
# lst_files.sort()

try:
    os.mkdir(output_prefix)
except FileExistsError:
    pass

def process(f : str):
    filename = input_prefix + sep + f
    ofilename = output_prefix + sep + f[:-3] + 'csv'
    with open(filename, 'rb') as ifile:
        icontents = ifile.read()
        with open(ofilename, 'wb') as ofile:
            ofile.write(b'\n')
            for l in icontents.splitlines():
                fields = l.strip().split(b' ')
                subfields = fields[:-1]
                ol = ( # fields[0] + sep_field +
                      sep_subfield.join(subfields) +
                      sep_field + fields[-1] + b'\n')
                ofile.write(ol)

if not use_threadpool:
    for f in lst_files: 
        process(f)
elif __name__ == '__main__':
    from multiprocessing import Pool
    with Pool(8) as tp:
        tp.map(process, lst_files)
