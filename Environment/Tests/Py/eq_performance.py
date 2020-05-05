import time

start = time.time()

a = {'string1', 'string2', 'string3', 'string4', 'string5'}
b = {'string1', 'string2', 'string3', 'string4', 'string5'}
for i in range(1000000):
    a == b

end = time.time()

print(int((end - start) * 1000))
