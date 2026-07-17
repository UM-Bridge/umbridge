import Pkg

tempdir = mktempdir()
Pkg.activate(tempdir)
Pkg.add(["UMBridge"])

using UMBridge

testmodel = UMBridge.Model(
    name = "forward",
    inputSizes = [1],
    outputSizes = [1],
    supportsGradient = true,
    evaluate = (input, config) -> [[2 * input[1][1]]],
    gradient = (outWrt, inWrt, input, sens, config) -> [2 * sens[1]]
)

UMBridge.serve_models([testmodel], 4242)
