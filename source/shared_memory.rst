.. _shared-memory:

=============================================
Client-Server Communication via Shared Memory
=============================================

UM-Bridge uses HTTP to communicate between an UQ client and a model server
by default. However, this is not the most performant option (in HPC) as it 
carries around 1 millisecond of overhead per request. We provide a quicker
alternative that uses shared memory (RAM) to transfer data. This approach 
works by using allocated memory buffers in RAM as the transfer medium for 
UM-Bridge inputs and outputs. To prevent data races in a parallel setting, 
each memory buffer is tagged with the ID of the thread making the request 
in the parallel client, and this ID is shared through a JSON entry so that 
the model can access the correct buffer. 

Currently, only the C++ and Python implementations are supported, and this
feature is off by default unless enabled through the ``use_shmem`` option
when instantiating the ``HTTPModel`` object. The code will first 
perform a test transfer using shared memory to check whether the client and
server are able to communicate; it reverts back to HTTP if this test fails.
