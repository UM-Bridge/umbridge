classdef TestMatlab < matlab.unittest.TestCase

    methods (Test)

        function testConnection(testCase)
            uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.evaluate([1, 3])
            exactValue = -5.147502395904501;
            testCase.verifyEqual(httpValue, exactValue, 'RelTol', 1e-14)

        end

        function testBasicExample(testCase)
          matlabClient;;
        end

        function testTTExample(testCase)
          ttClient;;
        end
    end
end
