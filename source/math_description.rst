.. _math-description:

=====================================
Mathematical abstraction in UM-Bridge
=====================================

In this section, we will describe UM-Bridge's interface mathematically. 

Let :math:`\mathbf{F}` denote the numerical model that maps the model input vector, :math:`\boldsymbol{\theta}` 
to the output vector :math:`\mathbf{F}(\boldsymbol{\theta})`. We will use bold font to 
indicate vectors. Note that both inputs and ouputs are required to be a list of lists in the actual 
implementation. For a list of :math:`d` input vectors each with :math:`n` dimensions, we have

.. math::    
    \mathbf{F}\, : \,
    \mathbb{R}^{n \times d}
    \;\longrightarrow\;
    \mathbb{R}^{m \times d}.

The arguments ``inWrt`` and ``outWrt`` in functions, where derivatives are involved, allow the user to 
select particular indices (out of :math:`d` indices) at which the derivative should be evaluated with 
respect to. However, more of this will be clarified in the respective sections.

Additionally, there may be an objective function :math:`L = L(\mathbf{F}(\boldsymbol{\theta}))`. 

UM-Bridge allows the following four operations.

Model Evaluation
================

This is simply the so called forward map that takes an element from the list of input vectors, 
:math:`\boldsymbol{\theta} = (\theta_1, \ldots, \theta_n) \in \mathbb{R}^n`, and returns the model output, 
:math:`\mathbf{F}(\boldsymbol{\theta}) = (F(\boldsymbol{\theta})_1, \ldots, F(\boldsymbol{\theta})_m) \in \mathbb{R}^m`.

For a collection of :math:`d` input vectors, each of dimension :math:`n`, this follows the definition as before:

.. math::
    
    \mathbf{F} : \mathbb{R}^{n \times d} \; \longrightarrow \; \mathbb{R}^{m \times d}.
    
In practice, both inputs and outputs are expected as lists of lists.


Gradient of the objective function
==================================

The gradient function evaluates the sensitivity of the scalar objective. Using the chain rule:

.. math::
    :name: eq:1
    
    \nabla_{\boldsymbol{\theta}}L
    = \left(\frac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}}\right)^{\!\top}
    \boldsymbol{\lambda},
    \qquad
    \boldsymbol{\lambda} = \frac{\partial L}{\partial \mathbf{F}},

where :math:`\boldsymbol{\lambda}` is known as the sensitivity vector and 
:math:`\dfrac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}}` is actually the Jacobian of the
forward map.

Since there are multiple choices due to the format of the input and output, we can select a specific
component within the input (:math:`\boldsymbol{\theta}_i \in \mathbb{R}^n`) and output list of 
lists (:math:`\mathbf{F}_j \in \mathbb{R}^m`). These indices are chosen using ``inWrt`` and ``outWrt``, respectively, 
in the implementation.

So :ref:`(1) <eq:1>` becomes

.. math::
    
    \nabla_{\boldsymbol{\theta}_i}
    = \left( \dfrac{\partial \mathbf{F}_j}{\partial \boldsymbol{\theta}_i} \right) ^ {\!\top}
    \boldsymbol{\lambda}_j,
    \qquad
    \boldsymbol{\lambda}_j = \dfrac{\partial L}{\partial \mathbf{F}_j},
    
where :math:`\boldsymbol{\lambda}_j` is the ``sens`` argument in the code. 

The output of this operation is a vector because we are essentially doing a matrix vector product.

Applying Jacobian to a vector
=============================

The apply Jacobian function evaluates the product of the transpose of the model's Jacobian, :math:`J^{\top}`, and a
vector, :math:`\mathbf{v}`, of the user's choice (``vec``). The Jacobian of a vector-valued function 
is given by
 
.. math::
    J =
    \frac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}} =
    \left[
    \begin{array}{ccc}
    \dfrac{\partial \mathbf{F}}{\partial \theta_1} & \cdots & \dfrac{\partial \mathbf{F}}{\partial \theta_n}
    \end{array}
    \right] = 
    \begin{pmatrix}
    \dfrac{\partial F_{1}}{\partial \theta_{1}} & \cdots &
    \dfrac{\partial F_{1}}{\partial \theta_{n}} \\[12pt]
    \vdots & \ddots & \vdots \\[4pt]
    \dfrac{\partial F_{m}}{\partial \theta_{1}} & \cdots &
    \dfrac{\partial F_{m}}{\partial \theta_{n}}
    \end{pmatrix}
    \in \mathbb{R}^{m \times n}.


For a chosen :math:`\mathbf{v} \in \mathbb{R}^{n}`, this is simply

.. math::
    J^{\!\top}\,\mathbf{v}
    = \left( \dfrac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}} \right) ^ {\!\top} \,\mathbf{v}.

Additionally, we can use this to express the gradient function by setting 
:math:`\mathbf{v} = \boldsymbol{\lambda}` as mentioned before.

However, as before, we can choose an index each from the input and output to construct the Jacobian such that
:math:`J_{ji} = \frac{\partial \mathbf{F}_j}{\partial \boldsymbol{\theta}_i}`. The output of this 
action is then

.. math::
    \texttt{output} =
    J_{ji}\,\mathbf{v}
    = \dfrac{\partial \mathbf{F}_j}{\partial \boldsymbol{\theta}_i}\,\mathbf{v},

where the :math:`i^{th}` and :math:`j^{th}` indices coresspond to ``inWrt`` and ``outWrt``.

Applying Hessian to a vector
============================

The apply Hessian action is a combination of the previous two sections: the action is still a matrix-vector product, but 
the matrix is the Hessian of an objective function. The Hessian, :math:`H`, is given by

.. math::
    H =
    \frac{\partial^2 L}{\partial \boldsymbol{\theta}\,\partial \boldsymbol{\theta}}
    = \frac{\partial}{\partial \boldsymbol{\theta}}
    \left(
    \frac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}}
    \right)^{\!\top}
    \boldsymbol{\lambda} = 
    \begin{bmatrix}
    \dfrac{\partial^2 L}{\partial \theta_1^2} & \dfrac{\partial^2 L}{\partial \theta_1 \partial \theta_2} & \cdots & \dfrac{\partial^2 L}{\partial \theta_1 \partial \theta_n} \\[18pt]
    \dfrac{\partial^2 L}{\partial \theta_2 \partial \theta_1} & \dfrac{\partial^2 L}{\partial \theta_2^2} & \cdots & \dfrac{\partial^2 L}{\partial \theta_2 \partial \theta_n} \\[18pt]
    \vdots & \vdots & \ddots & \vdots \\[6pt]
    \dfrac{\partial^2 L}{\partial \theta_n \partial \theta_1} & \dfrac{\partial^2 L}{\partial \theta_n \partial \theta_2} & \cdots & \dfrac{\partial^2 L}{\partial \theta_n^2}
    \end{bmatrix},

where :math:`L` is the objective function and :math:`\boldsymbol{\lambda}` is the sensitivity vector as defined previously.

So the product of :math:`H` and the chosen vector (of size :math:`n`) can be written as

.. math::
    H\,\mathbf{v}
    = \dfrac{\partial^2 L}{\partial \boldsymbol{\theta}\,\partial \boldsymbol{\theta}}\,\mathbf{v} = 
    \left[\dfrac{\partial}{\partial \boldsymbol{\theta}}
    \left(
    \dfrac{\partial \mathbf{F}}{\partial \boldsymbol{\theta}}
    \right)^{\!\top}
    \boldsymbol{\lambda}\right]\,\mathbf{v}.

As in the apply Jacobian action, we can select certain indices from the list of lists to construct the Hessian. 
Since :math:`H` contains the second derivative of :math:`L`, we require two indices from the input:
``inWrt1`` and ``inWrt2``. The output of this action is 

.. math::
    \texttt{output} =
    \left( \dfrac{\partial}{\partial \boldsymbol{\theta}_i}
    \left[ \left( \dfrac{\partial \mathbf{F}_k}{\partial \boldsymbol{\theta}_j} \right) ^ {\!\top} \, \boldsymbol{\lambda}_k \right] \right)
    \, \mathbf{v}.


