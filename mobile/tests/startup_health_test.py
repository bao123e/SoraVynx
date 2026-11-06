import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from startup_health import healthy_start


class StartupHealthTests(unittest.TestCase):
    def test_restarted_process_cannot_pass(self):
        samples = [{'pid': ''}, {'pid': ''}, {'pid': '6978'}]
        self.assertFalse(healthy_start('6772', samples, ''))

    def test_native_crash_rejects_even_if_final_sample_has_a_process(self):
        line = '11-06 10:30:35.059  6772  6884 E CRASH : signal 11 (SIGSEGV)'
        self.assertFalse(healthy_start('6772', [{'pid': '6772'}], line))

    def test_unrelated_process_crash_does_not_blame_the_game(self):
        line = '11-06 10:30:35.059  1000  1001 F libc : Fatal signal 11'
        self.assertTrue(healthy_start('6772', [{'pid': '6772'}]*3, line))

    def test_missing_initial_game_process_is_not_success(self):
        self.assertFalse(healthy_start('', [{'pid': '6978'}], ''))


if __name__ == '__main__': unittest.main()
