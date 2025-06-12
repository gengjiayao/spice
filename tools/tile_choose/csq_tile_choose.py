from matplotlib import pyplot as plt

# BBD400
x1 = [0, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64]
y1 = [0, 3766, 5456, 6664, 7265, 7595, 7824, 7962, 8030, 8056, 8090, 8108, 8120, 8129, 8132, 8136]

# BBD300
x2 = [0, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64]
y2 = [0, 4796, 6536, 7636, 8298, 8629, 8833, 8943, 9012, 9075, 9106, 9124, 9135, 9154, 9158, 9167]

plt.figure(figsize=(10, 6))
plt.plot(x1, y1)
plt.xlabel("Tile dims")
plt.ylabel("CSQ's numbers")
plt.title("BBD400")
plt.axhline(y=8143, color='r', linestyle='--', linewidth=1.5)
plt.savefig("bbd400.png")

plt.figure(figsize=(10, 6))
plt.plot(x2, y2)
plt.xlabel("Tile dims")
plt.ylabel("CSQ's numbers")
plt.title("BBD300")
plt.axhline(y=9227, color='r', linestyle='--', linewidth=1.5)
plt.savefig("bbd300.png")