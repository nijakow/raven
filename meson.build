project('raven', 'c')

src = [
'./src/raven/raven.c',
'./src/raven/channel.c',
'./src/main.c',
'./src/util/ringbuffer.c',
'./src/util/log.c',
'./src/util/memory.c',
'./src/util/wrap.c',
'./src/util/utf8.c',
'./src/util/stringbuilder.c',
'./src/vm/scheduler.c',
'./src/vm/interpreter.c',
'./src/vm/fiber.c',
'./src/vm/builtins.c',
'./src/vm/frame.c',
'./src/vm/op.c',
'./src/server/server.c',
'./src/serializer/serializer.c',
'./src/gc/stats.c',
'./src/gc/gc.c',
'./src/fs/filesystem.c',
'./src/fs/file.c',
'./src/core/blueprint.c',
'./src/core/base_obj.c',
'./src/core/type.c',
'./src/core/any.c',
'./src/core/object_table.c',
'./src/core/vars.c',
'./src/core/objects/mapping.c',
'./src/core/objects/connection.c',
'./src/core/objects/string.c',
'./src/core/objects/symbol.c',
'./src/core/objects/array.c',
'./src/core/objects/object.c',
'./src/core/objects/function.c',
'./src/core/objects/funcref.c',
'./src/lang/codewriter.c',
'./src/lang/compiler.c',
'./src/lang/script.c',
'./src/lang/parser.c',
'./src/lang/parsepiler.c',
'./src/lang/reader.c',
]

includes = include_directories('src')

cc = meson.get_compiler('c')

executable('raven.exe', sources: src, include_directories: includes, install: true)
