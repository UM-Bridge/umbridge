function istrue = supports_apply_jacobian(self)

supports = self.get_model_info();
istrue = supports.ApplyJacobian;

end
