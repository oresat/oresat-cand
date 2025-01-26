from argparse import ArgumentParser

from .client import NodeClient, Datatype


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
    subparser.add_argument("index", type=int_hex, help="od index to read from")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to read from")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")

    subparser = subparsers.add_parser("sdo-write", help="write a value to another node")
    subparser.add_argument("index", type=int_hex, help="od index to write to")
    subparser.add_argument("subindex", type=int_hex, help="od subindex to write to")
    subparser.add_argument("dtype", choices=dtypes, help="od datatype")
    subparser.add_argument("value", help="value to write")

    subparser = subparsers.add_parser("emcy-send", help="send a emcy message")
    subparser.add_argument("value", type=int_hex, help="emcy code")

    subparser = subparsers.add_parser("tpdo-send", help="send a tpdo message")
    subparser.add_argument("value", type=int, help="tpdo num")

    args = parser.parse_args()

    client = NodeClient(debug=args.verbose)

    try:
        if args.subcommand == "od-read":
            value = client.od_read(args.index, args.subindex, Datatype[args.dtype.upper()])
            print(value)
        elif args.subcommand == "od-write":
            client.od_write(args.index, args.subindex, Datatype[args.dtype.upper()], args.value)
        elif args.subcommand == "sdo-read":
            value = client.sdo_read(args.index, args.subindex, Datatype[args.dtype.upper()])
            print(value)
        elif args.subcommand == "sdo-write":
            client.sdo_write(args.index, args.subindex, Datatype[args.dtype.upper()], args.value)
        elif args.subcommand == "emcy-send":
            client.emcy_send(args.value)
        elif args.subcommand == "tpdo-send":
            client.tpdo_send(args.value)
    except Exception as e:
        print(f"ERROR: {e}")
