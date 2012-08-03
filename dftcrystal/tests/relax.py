def test():
  from lada.dftcrystal.relax import Relax
  from lada.dftcrystal.crystal import Crystal
  from lada.dftcrystal.basis import Shell

  a = Relax()
  a.optgeom.maxcycle = 10
  a.maxcycle = 300
  a.shrink = 4, 4
  a.basis['Si']    = \
    [ Shell('s',  a0=[149866.0, 0.0001215],
                  a1=[22080.6, 0.000977], 
                  a2=[4817.5, 0.0055181], 
                  a3=[1273.5, 0.0252], 
                  a4=[385.11, 0.0926563], 
                  a5=[128.429, 0.2608729], 
                  a6=[45.4475, 0.4637538], 
                  a7=[16.2589, 0.2952] ),
      Shell('sp', a0=[881.111, -0.0003, 0.0006809],
                  a1=[205.84, -0.005, 0.0059446],
                  a2=[64.8552, -0.0368, 0.0312],
                  a3=[23.9, -0.1079, 0.1084],
                  a4=[10.001, 0.0134, 0.2378],
                  a5=[4.4722, 0.3675, 0.3560066],
                  a6=[2.034, 0.5685, 0.341],
                  a7=[0.9079, 0.2065, 0.1326]),
      Shell('sp', 4.0, a0=[2.6668, -0.0491, 0.0465],
                       a1=[1.078, -0.1167, -0.1005],
                       a2=[0.3682, 0.23, -1.0329]),
      Shell('sp', 0.0, a0=[0.193, 1.0, 1.0]),
      Shell('d', 0.0, a0=[0.61, 1.0]) ]

  b = Crystal(216, 5.45)                          \
             .add_atom(0, 0, 0, 'Si')             \
             .add_atom(0.25, 0.25, 0.25, 'Si')

  
  a(b, '/local/bull/shit')
if __name__ == '__main__': test()
