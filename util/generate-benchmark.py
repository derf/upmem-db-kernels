#!/usr/bin/env python3

import argparse
import random


def make_read_op(op=None):
    if not op:
        op = random.choice("count select".split())
    pred = random.choice("lt le eq ne ge gt bs bc".split())

    if pred in ("bs", "bc"):
        pred_arg = int(random.random() * 33)
    else:
        pred_arg = int(random.random() * 2**34)

    return "{op_" + op + ", pred_" + pred + ", " + str(pred_arg) + "},"


def make_write_op(op=None):
    if not op:
        op = random.choice("update".split())

    if op == "update":
        pred_arg = int(random.random() * 2**34)
        return "{op_" + op + ", 0, " + str(pred_arg) + "},"


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__
    )
    parser.add_argument("--operation", type=str)
    parser.add_argument("--n-operations", type=int, default=200)
    parser.add_argument("--n-consecutive", type=int, default=200)
    parser.add_argument("--with-writes", action="store_true")
    args = parser.parse_args()

    print("struct benchmark_event benchmark_events[] = {")

    consecutive_count = 0
    have_valid_bitmask = False
    for i in range(args.n_operations):
        if consecutive_count == args.n_consecutive:
            print("{op_insert, 0, 1},")
            consecutive_count = 0
            have_valid_bitmask = False
        else:
            if args.with_writes and have_valid_bitmask and random.random() < 0.33:
                print(make_write_op())
            else:
                line = make_read_op(args.operation)
                if "select" in line:
                    have_valid_bitmask = True
                print(line)
            consecutive_count += 1

    print("};")


if __name__ == "__main__":
    main()
