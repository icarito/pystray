# coding=utf-8
import setuptools

# La metadata ahora está en pyproject.toml
# El código para encontrar los paquetes y otras lógicas de build se mantienen.

setuptools.setup(
    package_dir={'': 'lib'},
    packages=setuptools.find_packages(where='lib'),
    # 'install_requires' ya no es necesario aquí si lo pusiste en pyproject.toml
)
