classdef TestMatlab < matlab.unittest.TestCase

    methods (Test)

        function testConnection(testCase)
            uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.evaluate([1, 3])
            exactValue = -3.176886723809662;
            testCase.verifyEqual(httpValue, exactValue, 'RelTol', 1e-14)

        end

        function testApplyJacobian(testCase)
             uri = 'http://localhost:4243';

            model = HTTPModel(uri,'posterior');

            httpValue = model.apply_jacobian([1.0,4.0], [1,3], 0, 0)
            exactValue = -6.877474014398787;
            testCase.verifyEqual(httpValue, exactValue, 'RelTol', 1e-14)

        end
        
        function testBasicExample(testCase)
          matlabClient;
        end

        function testTTExample(testCase)
          ttClient;
        end
        
        function testSGMKExample(testCase)
            sgmkClient;
        end
        
    end
end
