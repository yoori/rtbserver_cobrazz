#!/usr/bin/python3.12

import json
import pathlib
import sys
import tempfile
import unittest


SOURCE_ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE_ROOT / 'AI' / 'lib'))

from segment_model.SegmentModelPublication import SegmentModelPublication


class SegmentModelPublicationTest(unittest.TestCase):
  def test_publishes_complete_model_by_directory_rename(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      publication = SegmentModelPublication(temp_dir, '20260901.120000')
      publication.start()
      self.assertTrue(publication.training_path.is_dir())
      publication.update_progress({
        'stage': 'structuring',
        'completed_batches': 7,
        'total_batches': 10,
      })
      self.write_required_files(publication.training_path)

      published_path = publication.publish({'segments_count': 2})

      self.assertFalse(publication.training_path.exists())
      self.assertEqual(
        pathlib.Path(temp_dir) / '20260901.120000',
        published_path)
      traits = json.loads((published_path / 'traits.json').read_text())
      self.assertEqual('published', traits['status'])
      self.assertEqual(7, traits['progress']['completed_batches'])
      self.assertEqual(2, traits['segments_count'])
      self.assertIn('train_end', traits)

  def test_does_not_publish_incomplete_model(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      publication = SegmentModelPublication(temp_dir, 'incomplete').start()

      with self.assertRaisesRegex(RuntimeError, 'training.json'):
        publication.publish()

      publication.interrupt('missing output')
      traits = json.loads(
        (publication.training_path / 'traits.json').read_text())
      self.assertEqual('interrupted', traits['status'])
      self.assertEqual('missing output', traits['interruption_reason'])

  def test_context_records_exception(self):
    with tempfile.TemporaryDirectory() as temp_dir:
      with self.assertRaisesRegex(ValueError, 'training failed'):
        with SegmentModelPublication(temp_dir, 'failed'):
          raise ValueError('training failed')
      traits = json.loads(
        (pathlib.Path(temp_dir) / '~failed' / 'traits.json').read_text())
      self.assertEqual('interrupted', traits['status'])
      self.assertEqual('training failed', traits['interruption_reason'])

  @staticmethod
  def write_required_files(model_path):
    for file_name, value in (
        ('config.json', {}),
        ('training.json', []),
        ('metrics.json', {}),
        ('segments.json', [])):
      (model_path / file_name).write_text(json.dumps(value))


if __name__ == '__main__':
  unittest.main()
