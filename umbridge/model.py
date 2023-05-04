from abc import ABC, abstractmethod

class Model(ABC):

    def __init__(self, name):
        self.name = name

    @abstractmethod
    def get_input_sizes(self):
        raise NotImplementedError(f'You need to implement this method in {self.__class__.__name__}.')

    @abstractmethod
    def get_output_sizes(self):
        raise NotImplementedError(f'You need to implement this method in {self.__class__.__name__}.')
    
    @abstractmethod
    def __call__(self, parameters, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')

    @abstractmethod
    def gradient(self, out_wrt, in_wrt, parameters, sens, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')
    
    @abstractmethod
    def apply_jacobian(self, out_wrt, in_wrt, parameters, vec, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')
    
    @abstractmethod
    def apply_hessian(self, out_wrt, in_wrt1, in_wrt2, parameters, sens, vec, config={}):
        raise NotImplementedError(f'Method called but not implemented in {self.__class__.__name__}.')

    def supports_evaluate(self):
        return False
    def supports_gradient(self):
        return False
    def supports_apply_jacobian(self):
        return False
    def supports_apply_hessian(self):
        return False