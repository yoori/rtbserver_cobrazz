#!/usr/bin/env python3

import importlib.util
import io
import os
import random
import subprocess
import sys
import tempfile
import unittest


ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCRIPT = os.path.join(ROOT, "CMS", "Plugin", "exec", "bin", "GenerateCpuAffinity.py")
PRODUCTION = os.path.join(
  ROOT, "tests", "Utils", "data", "CpuAffinity", "production.tsv")
CMS_CONFIG = os.path.join(
  ROOT, "tests", "Utils", "data", "CpuAffinity", "cms.xml")
CMS_PROFILE_XSL = os.path.join(ROOT, "CMS", "Plugin", "xslt", "CpuAffinityPools.xsl")

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("generate_cpu_affinity", SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def read(text):
  return module.read_pools(io.StringIO(text))


class GenerateCpuAffinityTest(unittest.TestCase):
  def assert_layout_is_dense(self, pools, layout):
    nodes = {}
    for pool in pools:
      nodes.setdefault(pool.numa_node, []).extend(layout[(pool.numa_node, pool.name)])
      self.assertEqual(pool.threads, len(layout[(pool.numa_node, pool.name)]))
    for positions in nodes.values():
      self.assertEqual(list(range(len(positions))), sorted(positions))

  def test_output_uses_list_position_as_thread_index(self):
    pools = read(
      "numa_node\tpool\tthreads\tpoints\n"
      "0\tservice/heavy\t2\t100\n"
      "0\tservice/light\t3\t10\n")
    layout = module.generate_layout(pools)
    output = io.StringIO()
    module.write_layout(output, pools, layout)

    lines = output.getvalue().splitlines()
    self.assertEqual("# pool\tnuma_node\tabstract_cpu_by_thread_index", lines[0])
    self.assertEqual(3, len(lines))
    positions = lines[1].split("\t")[2].split(",")
    self.assertEqual(2, len(positions))
    self.assertNotIn("service/", lines[1].split("\t")[0])
    self.assert_layout_is_dense(pools, layout)

  def test_output_dir_writes_one_file_per_service(self):
    pools = read(
      "numa_node\tpool\tthreads\tpoints\n"
      "0\tUserBindServer/ub-grpc-p\t2\t100\n"
      "0\tUserBindServer/rdb-batch\t3\t90\n"
      "1\tUserInfoManager/uim-grpc-p\t2\t80\n")
    layout = module.generate_layout(pools)

    with tempfile.TemporaryDirectory() as output_dir:
      module.write_service_layouts(output_dir, pools, layout)
      self.assertEqual(
        ["UserBindServer.cpu_aff", "UserInfoManager.cpu_aff"],
        sorted(os.listdir(output_dir)))

      with open(
        os.path.join(output_dir, "UserBindServer.cpu_aff"),
        "r",
        encoding="utf-8") as stream:
        lines = stream.read().splitlines()

    self.assertEqual("# pool\tnuma_node\tabstract_cpu_by_thread_index", lines[0])
    self.assertEqual("rdb-batch", lines[1].split("\t")[0])
    self.assertEqual("ub-grpc-p", lines[2].split("\t")[0])

  def test_layout_is_independent_of_input_order(self):
    pools = read(
      "numa_node\tpool\tthreads\tpoints\n"
      "0\tservice/a\t3\t80\n"
      "0\tservice/b\t7\t20\n"
      "1\tservice/c\t4\t50\n")
    self.assertEqual(
      module.generate_layout(pools),
      module.generate_layout(list(reversed(pools))))

  def test_thread_counts_remain_balanced_after_every_modulo(self):
    pools = read(
      "numa_node\tpool\tthreads\tpoints\n"
      "0\tservice/hot\t7\t100\n"
      "0\tservice/warm\t11\t30\n"
      "0\tservice/cold\t13\t1\n")
    layout = module.generate_layout(pools)
    for row in module.quality_rows(pools, layout):
      self.assertLessEqual(row["threads_max"] - row["threads_min"], 1)

  def test_rejects_duplicate_pool(self):
    with self.assertRaises(module.InputError):
      read(
        "numa_node\tpool\tthreads\tpoints\n"
        "0\tservice/pool\t2\t10\n"
        "0\tservice/pool\t3\t20\n")

  def test_generated_layout_is_never_worse_than_sequential(self):
    generator = random.Random(10681)
    for case in range(50):
      pools = [
        module.Pool(
          case % 2,
          f"service/pool-{index}",
          generator.randint(1, 10),
          generator.choice([0, 1, 5, 20, 50, 100]))
        for index in range(generator.randint(2, 8))
      ]
      generated = module.generate_layout(pools)
      sequential = module.sequential_layout(pools)
      self.assertLessEqual(
        module.collision_score(pools, generated),
        module.collision_score(pools, sequential))

  def test_production_regression(self):
    with open(PRODUCTION, "r", encoding="utf-8") as stream:
      pools = module.read_pools(stream)
    layout = module.generate_layout(pools)
    baseline = module.sequential_layout(pools)
    self.assert_layout_is_dense(pools, layout)

    threads_by_node = {}
    for pool in pools:
      threads_by_node[pool.numa_node] = threads_by_node.get(pool.numa_node, 0) + pool.threads
    self.assertEqual({0: 507, 1: 220}, threads_by_node)
    self.assertLess(
      module.collision_score(pools, layout),
      module.collision_score(pools, baseline) * 0.60)

    quality_at_36 = {
      row["numa_node"]: row
      for row in module.quality_rows(pools, layout)
      if row["cpu_count"] == 36
    }
    self.assertLess(quality_at_36[0]["max_overload_pct"], 20.0)
    self.assertLess(quality_at_36[1]["max_overload_pct"], 20.0)

  def test_cms_profile_uses_configured_thread_counts(self):
    xpath = (
      "/colo:colocation/application/"
      "serviceGroup[@descriptor='AdCluster']/"
      "serviceGroup[@descriptor='AdCluster/FrontendSubCluster']")
    result = subprocess.run(
      [
        "xsltproc",
        "--stringparam", "XPATH", xpath,
        "--stringparam", "HOST", "frontend.example",
        CMS_PROFILE_XSL,
        CMS_CONFIG,
      ],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      universal_newlines=True,
      check=True)
    pools = module.read_pools(io.StringIO(result.stdout))
    by_name = {pool.name: pool for pool in pools}

    expected = {
      "CampaignManager/cm-grpc-p": (1, 3),
      "CampaignManager/event_engine": (1, 16),
      "CampaignManager/cm-logging": (1, 2),
      "ChannelServer/cs-grpc-p": (0, 6),
      "ChannelServer/grpc-server": (0, 8),
      "UserInfoManager/uim-grpc-p": (1, 5),
      "UserInfoManager/event_engine": (1, 16),
      "UserInfoManager/rdb-batch": (1, 7),
      "UserBindServer/ub-grpc-p": (0, 4),
      "UserBindServer/grpc-server": (0, 6),
      "UserBindServer/rdb-batch": (0, 8),
      "FCGIActionServer/bid-request": (0, 40),
      "FCGIAdServer/bid-request": (0, 128),
      "FCGIRtbServer/bid-request": (0, 7),
      "FCGIRtbServer/fast-scheduler": (0, 3),
      "FCGIRtbServer/fcgi-accept": (0, 8),
      "FCGIRtbServer/grpc-asio-p": (0, 5),
      "FCGITrackServer/bid-request": (0, 42),
      "FCGIUserBindServer/bid-request": (0, 43),
    }
    self.assertEqual(59, len(pools))
    for name, (numa_node, threads) in expected.items():
      self.assertEqual(numa_node, by_name[name].numa_node, name)
      self.assertEqual(threads, by_name[name].threads, name)


if __name__ == "__main__":
  unittest.main()
