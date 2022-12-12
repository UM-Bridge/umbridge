import setuptools

with open("README.md", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="umbridge",
    version="1.2.1",
    author="UM-Bridge",
    author_email="",
    description="UM-Bridge (the UQ and Model Bridge) provides a unified interface for numerical models that is accessible from virtually any programming language or framework. It is primarily intended for coupling advanced models (e.g. simulations of complex physical processes) to advanced statistical or optimization methods.",
    long_description=long_description,
    long_description_content_type="text/markdown",
    install_requires=["aiohttp", "requests", "asyncio"],
    extras_require = {
        'pymc':  ["aesara"]
    },
    url="https://github.com/UM-Bridge/umbridge",
    packages=setuptools.find_packages(),
    classifiers=(                                 # Classifiers help people find your
        "Programming Language :: Python :: 3",    # projects. See all possible classifiers
        "License :: OSI Approved :: MIT License", # in https://pypi.org/classifiers/
        "Operating System :: OS Independent",
    ),
)
