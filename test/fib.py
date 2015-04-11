def fibonacci(n):
    if n <= 1:
        return n
    else:
        return fibonacci(n-1) + fibonacci(n-2)

sum = 0
for i in range(0,20):
    sum = sum + fibonacci(i)
print "Sum: %s" % sum
