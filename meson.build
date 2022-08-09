project('raven', 'c')

src = [
'./src/core/any.c',
'./src/core/blueprint.c',
'./src/core/objects/array.c',
'./src/core/objects/funcref.c',
'./src/core/objects/function.c',
'./src/core/objects/mapping.c',
'./src/core/objects/object.c',
'./src/core/objects/string.c',
'./src/core/objects/symbol.c',
'./src/core/objects/connection.c',
'./src/core/vars.c',
'./src/core/type.c',
'./src/core/base_obj.c',
'./src/core/object_table.c',
'./src/fs/file.c',
'./src/fs/filesystem.c',
'./src/gc/gc.c',
'./src/lang/codewriter.c',
'./src/lang/compiler.c',
'./src/lang/parsepiler.c',
'./src/lang/parser.c',
'./src/lang/reader.c',
'./src/main.c',
'./src/server/server.c',
'./src/util/log.c',
'./src/util/ringbuffer.c',
'./src/util/stringbuilder.c',
'./src/util/wrap.c',
'./src/util/memory.c',
'./src/vm/builtins.c',
'./src/vm/fiber.c',
'./src/vm/frame.c',
'./src/vm/scheduler.c',
'./src/vm/interpreter.c',
'./src/vm/op.c',
'./src/raven.c'
]

includes = include_directories('src')

cc = meson.get_compiler('c')

executable('raven.exe', sources: src, include_directories: includes, install: true)
