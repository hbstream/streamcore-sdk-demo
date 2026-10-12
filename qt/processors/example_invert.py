"""Minimal Processor module example; replace this with a model adapter."""

import numpy as np


def create_processor():
    def process_bgra(frame: np.ndarray) -> np.ndarray:
        output = frame.copy()
        output[:, :, :3] = 255 - output[:, :, :3]
        return output

    return process_bgra
