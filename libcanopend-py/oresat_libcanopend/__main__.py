from argparse import ArgumentParser

from . import NodeClient, Datatype, Entry


if __name__ == "__main__":
    dtypes = [i.name.lower() for i in list(Datatype)]

    def int_hex(arg: str) -> int:
        if arg.startswith("0x"):
            return int(arg, 16)
        return int(arg)

    parser = ArgumentParser()
    parser.add_argument("-v", "--verbose", action="store_true", help="verbose printing")
    subparsers = parser.add_subparsers(dest="subcommand", help="subcommand help")

    subparser = subparsers.add_parser("od-read", help="read a value from the od")
    subparser.add_argument("index", type=int_hex, help="od index to read from")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to read from")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")

    subparser = subparsers.add_parser("od-write", help="write a value to the od")
    subparser.add_argument("index", type=int_hex, help="od index to write to")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to write to")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")
    subparser.add_argument("value", help="value to write")

    subparser = subparsers.add_parser("sdo-read", help="read a value from another node")
    subparser.add_argument(
        "node_id", metavar="node-id", type=int_hex, help="id of the node to read from"
    )
    subparser.add_argument("index", type=int_hex, help="od index to read from")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to read from")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")

    subparser = subparsers.add_parser("sdo-write", help="write a value to another node")
    subparser.add_argument(
        "node_id", metavar="node-id", type=int_hex, help="id of the node to write to"
    )
    subparser.add_argument("index", type=int_hex, help="od index to write to")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to write to")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")
    subparser.add_argument("value", help="value to write")

    subparser = subparsers.add_parser("emcy-send", help="send a emcy message")
    subparser.add_argument("code", type=int_hex, help="emcy code")
    subparser.add_argument("info", type=int_hex, help="emcy info")

    subparser = subparsers.add_parser("tpdo-send", help="send a tpdo message")
    subparser.add_argument("num", type=int, help="tpdo num")

    args = parser.parse_args()

    node = NodeClient(debug=args.verbose)

    try:
        if args.subcommand == "od-read":
            entry = Entry(args.index, args.subindex, Datatype[args.dtype.upper()])
            value = node.od_read(entry)
            if isinstance(value, int):
                print(f"{value} 0x{value:X}")
            else:
                print(value)
        elif args.subcommand == "od-write":
            entry = Entry(args.index, args.subindex, Datatype[args.dtype.upper()])
            node.od_write(entry, args.value)
        elif args.subcommand == "sdo-read":
            entry = Entry(args.index, args.subindex, Datatype[args.dtype.upper()])
            value = node.sdo_read(args.node_id, entry)
            if isinstance(value, int):
                print(f"{value} 0x{value:X}")
            else:
                print(value)
        elif args.subcommand == "sdo-write":
            entry = Entry(args.index, args.subindex, Datatype[args.dtype.upper()])
            node.sdo_write(args.node_id, entry, args.value)
        elif args.subcommand == "emcy-send":
            node.send_emcy(args.code, args.info)
        elif args.subcommand == "tpdo-send":
            node.send_tpdo(args.num)
    except Exception as e:
        print(f"ERROR: {e}")
