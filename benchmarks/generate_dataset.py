import argparse
import json
import os
import random

CATEGORIES = ["phones", "cars", "furniture", "clothes", "realty"]

ITEMS_BY_CATEGORY = {
    "phones": ["iPhone 13", "Samsung galaxy S23", "Xiaomi redmi note"],
    "cars": ["BMW X5 2019", "Toyota Camry", "Lada Vesta"],
    "furniture": ["corner sofa", "wardrobe", "dresser"],
    "clothes": ["winter jacket", "nike sneakers", "summer dress"],
    "realty": ["2-room apartment", "studio", "room for rent"],
}

TITLE_PATTERNS = [
    "Sell {item}",
    "Buy {item}",
    "{item} in great condition",
]

SEMANTIC_QUERY_PRESETS = [
    {"text": "cheap smartphone", "category": "phones"},
    {"text": "family car", "category": "cars"},
    {"text": "cozy living room sofa", "category": "furniture"},
    {"text": "flat near subway", "category": "realty"},
]

def build_ad(rng, index):
    category = rng.choice(CATEGORIES)
    item = rng.choice(ITEMS_BY_CATEGORY[category])
    title = rng.choice(TITLE_PATTERNS).format(item=item)
    description = item + ". ad #" + str(index) + ". condition: used."

    return {
        "id": index,
        "title": title,
        "description": description,
        "category": category,
    }

def generate_ads(size, seed):
    rng = random.Random(seed)
    ads = []

    for i in range(size):
        ads.append(build_ad(rng, i))

    return ads

def pick_exact_text(rng, ad):
    words = ad["title"].split()
    for word in words:
        clean = word.strip().lower()
        if clean not in ("sell", "buy", "in", "great", "condition"):
            return clean
    return "phone"

def generate_queries(ads, seed, exact_count, semantic_count):
    rng = random.Random(seed + 1)
    queries = []
    query_id = 0

    for _ in range(exact_count):
        ad = rng.choice(ads)
        query_text = pick_exact_text(rng, ad)
        queries.append(
            {
                "id": query_id,
                "text": query_text,
                "profile": "exact",
                "category": None,
                "top_k": 10,
                "offset": 0,
                "relevant_ad_ids": [],
            }
        )
        query_id += 1

    for i in range(semantic_count):
        preset = SEMANTIC_QUERY_PRESETS[i % len(SEMANTIC_QUERY_PRESETS)]
        queries.append(
            {
                "id": query_id,
                "text": preset["text"],
                "profile": "semantic",
                "category": preset["category"],
                "top_k": 10,
                "offset": 0,
                "relevant_ad_ids": [],
            }
        )
        query_id += 1

    return queries

def fill_relevance(ads, queries):
    for query in queries:
        relevant = []

        if query["profile"] == "exact":
            needle = query["text"].lower()
            for ad in ads:
                haystack = (ad["title"] + " " + ad["description"]).lower()
                if needle in haystack:
                    relevant.append(ad["id"])
        else:
            for ad in ads:
                if ad["category"] == query["category"]:
                    relevant.append(ad["id"])

        query["relevant_ad_ids"] = relevant

def build_dataset(size, seed, exact_count, semantic_count):
    ads = generate_ads(size, seed)
    queries = generate_queries(ads, seed, exact_count, semantic_count)
    fill_relevance(ads, queries)

    return {
        "meta": {
            "version": 1,
            "seed": seed,
            "ad_count": len(ads),
            "query_count": len(queries),
        },
        "ads": ads,
        "queries": queries,
    }

def save_dataset(dataset, output_path):
    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(dataset, f, ensure_ascii=False, indent=2)

def parse_args():
    parser = argparse.ArgumentParser(description="Generate benchmark dataset JSON")
    parser.add_argument("--size", type=int, default=1000, help="Number of ads")
    parser.add_argument("--seed", type=int, default=42, help="Random seed")
    parser.add_argument("--exact", type=int, default=20, help="Exact queries count")
    parser.add_argument("--semantic", type=int, default=20, help="Semantic queries count")
    parser.add_argument(
        "--out",
        type=str,
        default="benchmarks/data/dataset.json",
        help="Output json path",
    )
    return parser.parse_args()

def main():
    args = parse_args()

    if args.size <= 0:
        raise ValueError("--size must be > 0")
    if args.exact < 0 or args.semantic < 0:
        raise ValueError("--exact and --semantic must be >= 0")

    dataset = build_dataset(args.size, args.seed, args.exact, args.semantic)
    save_dataset(dataset, args.out)

    print(
        "saved "
        + str(dataset["meta"]["ad_count"])
        + " ads, "
        + str(dataset["meta"]["query_count"])
        + " queries -> "
        + args.out
    )


if __name__ == "__main__":
    main()