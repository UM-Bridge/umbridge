classdef TestMatlab < matlab.unittest.TestCase

    methods (Test)

        function testConnection(testCase)
            uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.evaluate([1, 3])
            exactValue = -5.147502395904501;
            testCase.verifyEqual(httpValue, exactValue, 'RelTol', 1e-14)

        end

        function testApplyJacobian(testCase)
             uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.apply_jacobian([1.0,4.0], [1,3], 0, 0)
            exactValue = -3.370206919896928;
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
