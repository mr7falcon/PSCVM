import time

start = time.time()

array11 = []
array21 = []
for i in range(100) :
    dict1 = {}
    dict2 = {}
    for j in range(100) :
        array12 = []
        array22 = []
        for k in range(100) :
            array12.append("STRING")
            array22.append("STRING")
        dict1[j] = array12
        dict2[j] = array22
    array11.append(dict1)
    array21.append(dict2)

end = time.time()

print(int((end - start) * 1000))
