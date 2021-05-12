import math

TempB = -90
TempA = 90

L1 = 205
L2 = 205

TempA = TempA * math.pi / 180.0
TempB = TempB * math.pi / 180.0
PosX = L1 * math.cos(TempA) + L2 * math.cos(TempA + TempB)
PosY = L1 * math.sin(TempA) + L2 * math.sin(TempA + TempB)

print("X:{}, Y:{}".format(PosX, PosY))
