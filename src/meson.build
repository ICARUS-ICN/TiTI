cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

titi_sources = [
    'main.c', 
    'init.c', 
    'commands.c',
    'util.c',
    'metrics.c',
    'event.c',
    'stats.c',
    'print.c',
    'distrib.c',
    'network.c',
    'fddi.c',
    'fast.c',
    'promedios.c',
    'globals.c',
    'gg1.c'
]

executable('titi', titi_sources, dependencies : m_dep)
