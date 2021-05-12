import math

x = -66
y = 106
# x = 180
# y = 190
# x = 77
# y = 320

x = x + 205
y = y + 205

l1 = 205
l2 = 205

xy = x * x + y * y
l1pl2 = l1 * l1 + l2 * l2
l1sl2 = l1 * l1 - l2 * l2

b = math.acos((xy - l1pl2) * 1.0 / (2 * l1 * l2))
# print((xy + l1sl2) * 1.0 / (2 * l1 * math.sqrt(xy)))
a = math.atan2(y, x) + math.acos((xy + l1sl2) * 1.0 / (2 * l1 * math.sqrt(xy)))
b = b * 1.0 / math.pi * 180.0
a = a * 1.0 / math.pi * 180.0

b = 90 - b
a = 90 - a
print("B-Q96-#2 = {}".format(b))
print("A-Q95-#3 = {}".format(a))
