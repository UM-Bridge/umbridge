uri = 'http://localhost:4243';

model = HTTPModel(uri,'posterior');

httpValue = model.evaluate([1, 3])
