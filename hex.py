from itertools import combinations
# for i in range(0xF0, 0x100):
#     while i > 0:
#         print(str(hex(i)).upper().replace("X", "x"))
#         i -= 0x10

def rev_row(i):
    row_len = 0x10
    num_rows = 12
    top_row = row_len * (num_rows - 1)
    return top_row + i - 2 * (i - i % row_len)

for i in range(-0x77, 0x78):
    offset = i + 0x77
    if offset and offset % 0x0F == 0:
        print()
    print(str(hex(i))
        .upper()
        .replace("X", "x")
        .ljust(5 if i < 0 else 4, "0")
        , end=",")
print()

# capture from B1 -> H7
#             0x23 -> 0x89
# 0x23 - 0x89 = -0x66
codes = []
for i in range(0x99 + 1):
    if (i - 0x22) & 0x88: continue
    codes.append(i)

for i in codes:
    print(hex(i))

deltas = set()
for i, j in combinations(codes, 2):
    deltas.add(i - j)
    deltas.add(j - i)
print(deltas, len(deltas))
