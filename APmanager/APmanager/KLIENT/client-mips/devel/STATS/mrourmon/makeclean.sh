#!/usr/local/bin/bash
# restore ourmon directory to initial pre-configure status
# does nothing about web directory, only removes symlink

rm -fr bin tmp rrddata logs
rm -fr src/ourmon/*.o src/ourmon/ourmon src/ourmon/monconfig
rm -fr src/web.code/drawtopn
rm web.pages
