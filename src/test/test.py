import matplotlib.pyplot as plt

xyc = range(20)
colors = [1, 2, 5, 1, 9, 1, 8, 10, 15, 4, 13 ,17, 8, 13, 15, 2, 19,];
print xyc

plt.subplot(111)
plt.scatter(xyc[:16], xyc[:16], c=colors[:16], s=35, vmin=0, vmax=20)
plt.colorbar()
plt.xlim(0, 20)
plt.ylim(0, 20)
plt.title('Relative Error')

fig1 = plt.gcf()
fig1.savefig('image.png', dpi=100)
plt.show()
