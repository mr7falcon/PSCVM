import time

start = time.time()

haystack = 'vlaurvuibravuberstringgnseurgbruthjjhoi5n'
needle = 'string'
for i in range(1000000):
    haystack.find(needle)    

end = time.time()

print(int((end - start) * 1000))