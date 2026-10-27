"""Publication invariants; uses only Python's standard library."""
from pathlib import Path
import re
import unittest
ROOT = Path(__file__).resolve().parents[1]

class PublicationTests(unittest.TestCase):
    def test_floor_has_no_ble_stack(self):
        text = (ROOT / 'floorheating.yaml').read_text()
        for component in ('jk_bms_ble', 'ble_client', 'esp32_ble_tracker', 'esp32_ble'):
            self.assertNotIn(component, text)
        for room in ('bathroom', 'kitchen', 'livingroom', 'bedroom', 'corridor'):
            self.assertIn('id: ' + room + '_climate', text)
        self.assertIn('remote_id: floor_circulation_request_value', (ROOT / 'HVACSolarHeater.yaml').read_text())

    def test_public_credentials(self):
        for name in ('floorheating.yaml', 'HVACSolarHeater.yaml'):
            text = (ROOT / name).read_text()
            self.assertRegex(text, r'auth:\s*\n\s*username: admin\s*\n\s*password: admin')
            for line in text.splitlines():
                if re.match(r'\s*(password|key):', line):
                    self.assertTrue('!secret ' in line or line.strip() == 'password: admin', name)

if __name__ == '__main__':
    unittest.main()
