"""Evaluate one uninterrupted game launch, including native crash evidence."""
import re


def healthy_start(initial_pid, samples, runtime_text):
    if not initial_pid or not samples:
        return False
    if any(sample.get('pid') != initial_pid for sample in samples):
        return False
    for line in runtime_text.splitlines():
        match = re.match(r'^\S+\s+\S+\s+(\d+)\s+', line)
        if match and match.group(1) == initial_pid and re.search(
            r'Fatal signal|signal (?:6|7|11)\b|FATAL EXCEPTION', line, re.I
        ):
            return False
    return True
