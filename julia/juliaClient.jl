using UMBridge

url = "http://localhost:4242"

val = UMBridge.evaluate(url, "forward" ,[[1,3]], Dict())

print(val)

