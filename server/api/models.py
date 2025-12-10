from pydantic import BaseModel
from typing import List, Optional


class GenerateRequest(BaseModel):
    prompt: Optional[str] = None
    bpm: float
    key: Optional[str] = None
    measures: Optional[int] = 4
    generation_duration: Optional[float] = 6.0
    sample_rate: Optional[float] = 48000.00
    use_image: Optional[bool] = False
    image_base64: Optional[str] = None
    image_temperature: Optional[float] = 0.7
    keywords: Optional[List[str]] = []
