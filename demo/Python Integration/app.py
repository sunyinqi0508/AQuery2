import matplotlib.pyplot as plt

def draw(x, y):
    plt.figure()
    
    plt.plot(x.cast('i', shape=(len(x)//4, )), 
             y.cast('i', shape=(len(y)//4, )))
    plt.show()
 