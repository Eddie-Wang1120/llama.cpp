import numpy as np

def transform_to_i2(x):
    x_num = np.prod(x.shape)
    x = np.reshape(x, x_num)
    for i in range(x_num):
        if x[i] != 0:
            d = x[i]
            break
    x = np.divide(x, d)
    x = x.astype(np.uint8)
    x = np.reshape(x, [x.shape[0] // 4, 4])
    keep_bit = {0:192, 1:48, 2:12, 3:3}
    ans = np.zeros([x_num // 4], dtype=np.uint8)
    for i in range(4):
        x_bit_col = x[:, i]
        x_bit_shift = np.left_shift(x_bit_col, 6 - i * 2)
        x_bit_shift = np.bitwise_and(x_bit_shift, keep_bit[i])
        ans = np.bitwise_or(ans, x_bit_shift)
    return ans

x = np.random.randn(8, 1)
w = []
for i in range(64):
     w.append(np.random.randint(-1, 2))
w = np.array(w).reshape(8, 8)*2.26


# print(x)
print(w)

out = np.matmul(w, x)
# print(out)

w_trans = transform_to_i2(w)
print(w_trans)

col_num = 8

for i in range(2):
    weight = w_trans[i]
    for j in range(4):
        pos = (weight >> (6 - 2 * j)) & (3)
        print(pos)
        

# print(x)
# x = x.astype(np.uint8)
# assert(np.prod(x.shape) % 4 == 0)

# # print(np.prod(x.shape))
# x_num = np.prod(x.shape)
# x = np.reshape(x, x_num)

# # group_num = x_num // 4
# # vec = []
# # for group in range(group_num):
# #     temp = np.array(0).astype(np.int8)
# #     for num in range(4):
# #         if (x[group * 4 + num] == 1):
# #             temp = np.left_shift(temp, 1)
# #             temp = np.bitwise_or(temp, 0)
# #             temp = np.left_shift(temp, 1)
# #             temp = np.bitwise_or(temp, 1)
# #         elif (x[group * 4 + num] == -1):
# #             temp = np.left_shift(temp, 1)
# #             temp = np.bitwise_or(temp, 1)
# #             temp = np.left_shift(temp, 1)
# #             temp = np.bitwise_or(temp, 1)
# #         else :
# #             temp = np.left_shift(temp, 2)
# #         # print(temp)
# #         # 
# #         # 
# #     vec.append(temp)
# # res = np.array(vec).astype(np.uint8)
# # print(res)

# # x_reshape = np.reshape(x, [group_num, 4])
# # x_bits = np.unpackbits(x)
# # print(x_bits)
# x = np.reshape(x, [x.shape[0] // 4, 4])
# keep_bit = {0:192, 1:48, 2:12, 3:3}
# ans = np.zeros([x_num // 4], dtype=np.uint8)
# for i in range(4):
#     x_bit_col = x[:, i]
#     x_bit_shift = np.left_shift(x_bit_col, 6 - i * 2)
#     x_bit_shift = np.bitwise_and(x_bit_shift, keep_bit[i])
#     ans = np.bitwise_or(ans, x_bit_shift)
    
# print(ans)
# print(x_reshape)
# x_bits_0 = x_reshape[:, 0]
# x_bits_1 = x_reshape[:, 1]
# x_bits_2 = x_reshape[:, 2]
# x_bits_3 = x_reshape[:, 3]
# x_bits_0_shift = np.left_shift(x_bits_0, 6)
# print(x_bits_0_shift)
# x_bits_0_shift = np.bitwise_and(x_bits_0_shift, 192)
# print(x_bits_0_shift)
# x_bits_1_shift = np.left_shift(x_bits_1, 4)
# print(x_bits_1_shift)
# x_bits_1_shift = np.bitwise_and(x_bits_1_shift, 48)
# print(x_bits_1_shift)
# x_bits_2_shift = np.left_shift(x_bits_2, 2)
# print(x_bits_2_shift)
# x_bits_2_shift = np.bitwise_and(x_bits_2_shift, 12)
# print(x_bits_2_shift)
# x_bits_3_shift = np.left_shift(x_bits_3, 0)
# print(x_bits_3_shift)
# x_bits_3_shift = np.bitwise_and(x_bits_3_shift, 3)
# print(x_bits_3_shift)
# x_final = np.bitwise_or(np.bitwise_or(np.bitwise_or(x_bits_0_shift, x_bits_1_shift), x_bits_2_shift), x_bits_3_shift)
# print(x_final)