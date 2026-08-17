import json
import os
import tempfile
import unittest

import numpy as np

from opfppy.colormap import (
    build_palette,
    build_label_legend,
    export_label_legend,
    label_rgb,
    labels_to_rgb_array,
    load_colormap,
)


class TestColormapModes(unittest.TestCase):
    def test_round_robin_repeats_palette(self):
        cmap = [(10, 0, 0), (0, 20, 0), (0, 0, 30)]
        self.assertEqual(label_rgb(1, n_labels=10, mode="round-robin", cmap=cmap), (10, 0, 0))
        self.assertEqual(label_rgb(4, n_labels=10, mode="round-robin", cmap=cmap), (10, 0, 0))
        self.assertEqual(label_rgb(5, n_labels=10, mode="round-robin", cmap=cmap), (0, 20, 0))

    def test_rounding_robin_alias_is_accepted(self):
        cmap = [(1, 2, 3), (4, 5, 6)]
        self.assertEqual(label_rgb(3, n_labels=8, mode="rounding-robin", cmap=cmap), (1, 2, 3))

    def test_round_robin_mode_with_trailing_space_is_accepted(self):
        cmap = [(9, 8, 7), (6, 5, 4)]
        self.assertEqual(label_rgb(1, n_labels=2, mode="round-robin ", cmap=cmap), (9, 8, 7))

    def test_stretch_spreads_labels_across_full_extent(self):
        cmap = [
            (0, 0, 0),
            (10, 10, 10),
            (20, 20, 20),
            (30, 30, 30),
            (40, 40, 40),
            (50, 50, 50),
        ]
        # With 3 labels over 6 colors, the middle label lands halfway between
        # entries 2 and 3, yielding an interpolated gray.
        self.assertEqual(label_rgb(1, n_labels=3, mode="stretch", cmap=cmap), (0, 0, 0))
        self.assertEqual(label_rgb(2, n_labels=3, mode="stretch", cmap=cmap), (25, 25, 25))
        self.assertEqual(label_rgb(3, n_labels=3, mode="stretch", cmap=cmap), (50, 50, 50))

    def test_stretch_interpolates_between_palette_entries(self):
        cmap = [(255, 0, 0), (0, 0, 255)]
        self.assertEqual(label_rgb(1, n_labels=5, mode="stretch", cmap=cmap), (255, 0, 0))
        self.assertEqual(label_rgb(3, n_labels=5, mode="stretch", cmap=cmap), (128, 0, 128))
        self.assertEqual(label_rgb(5, n_labels=5, mode="stretch", cmap=cmap), (0, 0, 255))

    def test_stretch_requires_nlabels_for_single_lookup(self):
        cmap = [(0, 0, 0), (255, 255, 255)]
        with self.assertRaises(ValueError):
            label_rgb(1, mode="stretch", cmap=cmap)

    def test_build_palette_stretch_hits_last_color(self):
        cmap = [(0, 0, 0), (100, 0, 0), (200, 0, 0), (255, 0, 0)]
        out = build_palette(2, mode="stretch", cmap=cmap)
        self.assertEqual(out[0], (0, 0, 0))
        self.assertEqual(out[1], (255, 0, 0))

    def test_build_palette_stretch_creates_new_colors(self):
        cmap = [(255, 0, 0), (0, 0, 255)]
        out = build_palette(5, mode="stretch", cmap=cmap)
        self.assertEqual(out[0], (255, 0, 0))
        self.assertEqual(out[2], (128, 0, 128))
        self.assertEqual(out[4], (0, 0, 255))

    def test_labels_to_rgb_array_stretch_infers_nlabels(self):
        cmap = [(0, 0, 0), (100, 0, 0), (200, 0, 0), (255, 0, 0)]
        arr = labels_to_rgb_array([0, 1, 2, 4], mode="stretch", cmap=cmap)
        self.assertEqual(arr.shape, (4, 3))
        self.assertEqual(arr.dtype, np.float32)
        # label 0 is reserved white
        self.assertTrue(np.array_equal(arr[0], np.array([255.0, 255.0, 255.0], dtype=np.float32)))
        # label 4 (max label) should map to the last palette entry in stretch mode
        self.assertTrue(np.array_equal(arr[3], np.array([255.0, 0.0, 0.0], dtype=np.float32)))


class TestColormapLoading(unittest.TestCase):
    def test_load_colorcet_name(self):
        cmap = load_colormap("glasbey_hv")
        self.assertGreater(len(cmap), 0)
        self.assertEqual(len(cmap[0]), 3)
        self.assertTrue(all(isinstance(v, int) for v in cmap[0]))

    def test_load_hex_file(self):
        with tempfile.NamedTemporaryFile("w", suffix=".hex", delete=False, encoding="utf-8") as f:
            f.write("#ff0000\n")
            f.write("00ff00\n")
            path = f.name
        try:
            cmap = load_colormap(path)
            self.assertEqual(cmap, [(255, 0, 0), (0, 255, 0)])
        finally:
            os.unlink(path)


class TestColormapLegend(unittest.TestCase):
    def test_build_label_legend_from_labels(self):
        cmap = [(255, 0, 0), (0, 0, 255)]
        legend = build_label_legend(labels=[3, 1, 3, 2], mode="stretch", cmap=cmap)
        self.assertEqual([entry["label"] for entry in legend], [1, 2, 3])
        self.assertEqual(legend[0]["rgb"], [255, 0, 0])
        self.assertEqual(legend[1]["rgb"], [128, 0, 128])
        self.assertEqual(legend[2]["rgb"], [0, 0, 255])

    def test_export_label_legend_json(self):
        cmap = [(255, 0, 0), (0, 255, 0), (0, 0, 255)]
        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as f:
            path = f.name
        try:
            export_label_legend(
                path=path,
                labels=[1, 2, 3],
                mode="round-robin",
                cmap=cmap,
                colormap_source="unit-test",
            )
            with open(path, "r", encoding="utf-8") as f:
                payload = json.load(f)
            self.assertEqual(payload["mode"], "round-robin")
            self.assertEqual(payload["colormap_source"], "unit-test")
            self.assertEqual(payload["entries"][0]["hex"], "#ff0000")
        finally:
            os.unlink(path)

    def test_export_label_legend_csv(self):
        cmap = [(255, 0, 0), (0, 0, 255)]
        with tempfile.NamedTemporaryFile("w", suffix=".csv", delete=False, encoding="utf-8") as f:
            path = f.name
        try:
            export_label_legend(path=path, labels=[1, 2], mode="stretch", cmap=cmap)
            with open(path, "r", encoding="utf-8") as f:
                text = f.read()
            self.assertIn("label,hex,r,g,b", text)
            self.assertIn("1,#ff0000,255,0,0", text)
            self.assertIn("2,#0000ff,0,0,255", text)
        finally:
            os.unlink(path)

    def test_load_csv_file(self):
        with tempfile.NamedTemporaryFile("w", suffix=".csv", delete=False, encoding="utf-8") as f:
            f.write("255,0,0\n")
            f.write("0,1,0\n")  # float range entry
            path = f.name
        try:
            cmap = load_colormap(path)
            self.assertEqual(cmap, [(255, 0, 0), (0, 255, 0)])
        finally:
            os.unlink(path)

    def test_load_json_file(self):
        payload = {"colors": ["#0000ff", [1.0, 0.0, 0.0]]}
        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as f:
            json.dump(payload, f)
            path = f.name
        try:
            cmap = load_colormap(path)
            self.assertEqual(cmap, [(0, 0, 255), (255, 0, 0)])
        finally:
            os.unlink(path)


if __name__ == "__main__":
    unittest.main()
