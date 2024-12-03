#!/usr/bin/env python3

import argparse
import random


def make_read_op():
    op = random.choice("count select".split())
    pred = random.choice("lt le eq ne ge gt bs bc".split())

    if pred in ("bs", "bc"):
        pred_arg = int(random.random() * 33)
    else:
        pred_arg = int(random.random() * 2**34)

    return "{op_" + op + ", pred_" + pred + ", " + str(pred_arg) + "},"


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__
    )
    parser.add_argument("--n-operations", type=int, default=200)
    parser.add_argument("--n-consecutive", type=int, default=200)
    parser.add_argument("--with-writes", action="store_true")
    args = parser.parse_args()

    print("struct benchmark_event benchmark_events[] = {")

    consecutive_count = 0
    for i in range(args.n_operations):
        if consecutive_count == args.n_consecutive:
            print("{op_insert, (predicates)0, 1},")
            consecutive_count = 0
        else:
            print(make_read_op())
            consecutive_count += 1

    print("};")


if __name__ == "__main__":
    main()
