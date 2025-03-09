#!/usr/bin/env python3

from oresat_configs.scripts.gen_canopend import write_canopend_python

write_canopend_python(
    card_name="sw_common",
    config_file_path="../od.yaml",
    gen_dir_path="oresat_libcanopend/gen",
    base="",
)
