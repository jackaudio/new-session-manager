project('new-session-manager', 'c', 'cpp', version : '1.3.0', license : 'GPLv3')

liblodep = dependency('liblo') #and not 'lo'
threaddep = dependency('threads') 
jackdep = dependency('jack') #and not 'libjack'

executable('nsmd',
    sources: ['src/nsmd.cpp', 'src/debug.cpp', 'src/Endpoint.cpp', 'src/file.cpp', 'src/Thread.cpp'],
    dependencies: [liblodep, threaddep],
    install: true,
    )

executable('jackpatch',
    'src/jackpatch.c',
    dependencies: [liblodep, jackdep],
    install: true,
    )



