classdef TestMatlab < matlab.unittest.TestCase

    methods (Test)

        function testConnection(testCase)
            uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.evaluate([1, 3])
            exactValue = -11.194036030183454;
            testCase.verifyEqual(httpValue, exactValue, 'RelTol', 1e-14)

        end

        function testExample(testCase)
          matlabClient;;
        end
    end
end
