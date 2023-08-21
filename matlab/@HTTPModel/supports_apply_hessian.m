function istrue = supports_apply_hessian(self)

supports = self.get_model_info();
istrue = supports.ApplyHessian;

end
