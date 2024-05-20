FROM julia:latest

COPY minimal-server.jl /minimal-server.jl 

RUN julia -O3 -e 'using Pkg; Pkg.REPLMode.pkgstr("add UMBridge   ;precompile"); using UMBridge'

CMD julia /minimal-server.jl
