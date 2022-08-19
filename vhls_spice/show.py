import matplotlib.pyplot as plt
import numpy as np
import sys
name = sys.argv[1]
col = int(sys.argv[2])
data = np.loadtxt(name)
plt.subplot(111)
plt.plot(data[:,col])
#plt.savefig('res.png')
plt.show()
