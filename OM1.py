#!/usr/bin/python3
import scipy
import scipy.integrate as integrate
from scipy.integrate import quad
import numpy
from numpy import inf,exp,sign,abs

def f_O(x,m,alfa):
    return alfa*pow(x/m+1,-alfa)/(x+m)

def Laplace(t,s,m,alfa):
    return exp(-s*t)*f_O(t,m,alfa)

def sigma_Laplace(sigma, mu, m, alfa):
    s=mu-mu*sigma
    return sigma - quad(Laplace,0,numpy.inf,args=(s,m,alfa))[0]

def root_sigma_Laplace(mu,m,alfa):
    a=0.0
    Ea=sigma_Laplace(a,mu,m,alfa)
    Eb=Ea
    b=0.0
    i=2.0
    while sign(Ea)==sign(Eb):
        a=b
        Ea=Eb
        b=1-1/i
        Eb=sigma_Laplace(b,mu,m,alfa)
        i*=2
    # print("a=",a,", Ea=",Ea,", b=",b,", Eb=",Eb,sep="")
    prevx=0.0
    while True:
        if b==a:
            x=a
            break
        x=-Ea*(b-a)/(Eb-Ea)+a
        Ex=sigma_Laplace(b,mu,m,alfa)
        if abs(prevx-x)/x < 1E-16:
            break
        prevx=x
        if sign(Ex)!=sign(Ea):
            b=x
            Eb=Ex
        else:
            a=x
            Ea=Ex
    return x

sigma=0;mu=2;m=1;alfa=2
# print ("sigma_Laplace(sigma=",sigma,",mu=",mu,",m=",m,",alfa=",alfa,")=",sigma_Laplace(sigma,mu,m,alfa), sep="")
# sigma=0.999
# print ("sigma_Laplace(sigma=",sigma,",mu=",mu,",m=",m,",alfa=",alfa,")=",sigma_Laplace(sigma,mu,m,alfa), sep="")
# root_sigma_Laplace(mu,m,alfa)

mu=2;Upsilon=1
# Tiempos desmesurados con 1.01: Sigma>1-1E-13, W>1E13
# for alfa in (1.01,1,1.2,1.4,1.6,1.8,2,4,8):
for alfa in (1.4,1.7,2,4):
    m=Upsilon*(alfa-1)
    sigma=root_sigma_Laplace(mu,m,alfa)
    W=sigma/mu/(1-sigma)
    print("root_sigma_Laplace(mu=",mu,",m=",m,",alfa=",alfa,")=",sigma," E(W)=",W,sep="")
