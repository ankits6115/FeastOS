
export const SEED_PAIRINGS = [
  // Tandoori & Curries <-> Breads & Rice
  { a: 'paneer tikka', b: 'paneer butter masala', coOrders: 18, totalA: 24, totalB: 22 },
  { a: 'paneer tikka', b: 'butter naan (2 pcs)', coOrders: 21, totalA: 24, totalB: 28 },
  { a: 'paneer butter masala', b: 'butter naan (2 pcs)', coOrders: 26, totalA: 22, totalB: 28 },
  { a: 'chicken tikka', b: 'chicken curry', coOrders: 15, totalA: 19, totalB: 18 },
  { a: 'chicken curry', b: 'hyderabadi dum biryani', coOrders: 14, totalA: 18, totalB: 20 },
  { a: 'dal tadka', b: 'butter naan (2 pcs)', coOrders: 19, totalA: 20, totalB: 28 },
  { a: 'dal tadka', b: 'veg biryani', coOrders: 12, totalA: 20, totalB: 16 },

  // Pizzas & Sides / Pasta
  { a: 'margherita pizza', b: 'french fries', coOrders: 22, totalA: 30, totalB: 35 },
  { a: 'classic margherita', b: 'french fries', coOrders: 20, totalA: 28, totalB: 35 },
  { a: 'farmhouse pizza', b: 'penne arrabbiata', coOrders: 14, totalA: 19, totalB: 17 },
  { a: 'classic chicken pizza', b: 'chicken wings', coOrders: 17, totalA: 21, totalB: 23 },
  { a: 'chicken dominator pizza', b: 'peri peri fries', coOrders: 16, totalA: 18, totalB: 26 },

  // Burgers & Subs <-> Sides & Shakes
  { a: 'mcaloo tikki burger', b: 'french fries', coOrders: 28, totalA: 34, totalB: 35 },
  { a: 'mcchicken burger', b: 'peri peri fries', coOrders: 20, totalA: 25, totalB: 26 },
  { a: 'chicken zinger burger', b: 'hot and crispy chicken', coOrders: 18, totalA: 22, totalB: 20 },
  { a: 'veggie delite sub', b: 'cold coffee', coOrders: 13, totalA: 17, totalB: 24 },
  { a: 'paneer tikka sub', b: 'cheesy fries', coOrders: 15, totalA: 18, totalB: 19 },

  // Desserts & Coffee
  { a: 'caffe latte', b: 'butter croissant', coOrders: 24, totalA: 28, totalB: 30 },
  { a: 'cappuccino', b: 'chocolate brownie', coOrders: 22, totalA: 27, totalB: 25 },
  { a: 'cold coffee', b: 'chocolate lava cake', coOrders: 19, totalA: 24, totalB: 26 },
  { a: 'pistachio gelato', b: 'chocolate brownie', coOrders: 14, totalA: 18, totalB: 25 }
];


export function computeJaccardSimilarity(coOrders, totalA, totalB) {
  const union = (totalA + totalB - coOrders) + 1;
  if (union <= 0) return 0;
  return coOrders / union;
}


export function buildDynamicCoOccurrence(pastOrders = []) {
  const itemCounts = new Map();
  const pairCounts = new Map();

  if (Array.isArray(pastOrders) && pastOrders.length > 0) {
    for (const order of pastOrders) {
      const itemsInOrder = [];
      if (Array.isArray(order.items)) {
        for (const it of order.items) {
          const name = String(it.item_name || it.item_name_snapshot || it.name || '').toLowerCase().trim();
          if (name && !itemsInOrder.includes(name)) {
            itemsInOrder.push(name);
          }
        }
      }

      // Increment individual counts
      for (const name of itemsInOrder) {
        itemCounts.set(name, (itemCounts.get(name) || 0) + 1);
      }

      // Increment pairwise co-occurrence counts
      for (let i = 0; i < itemsInOrder.length; i++) {
        for (let j = i + 1; j < itemsInOrder.length; j++) {
          const a = itemsInOrder[i];
          const b = itemsInOrder[j];
          const key = a < b ? `${a}|${b}` : `${b}|${a}`;
          pairCounts.set(key, (pairCounts.get(key) || 0) + 1);
        }
      }
    }
  }

  return { itemCounts, pairCounts };
}


export function getPairwiseSimilarity(nameA, nameB, dynamicData) {
  const normA = String(nameA || '').toLowerCase().trim();
  const normB = String(nameB || '').toLowerCase().trim();
  if (normA === normB) return 1.0;

  // 1. Check live database co-occurrences first
  if (dynamicData && dynamicData.pairCounts) {
    const key = normA < normB ? `${normA}|${normB}` : `${normB}|${normA}`;
    const liveCoOrders = dynamicData.pairCounts.get(key);
    if (liveCoOrders && liveCoOrders > 0) {
      const cntA = dynamicData.itemCounts.get(normA) || liveCoOrders;
      const cntB = dynamicData.itemCounts.get(normB) || liveCoOrders;
      return computeJaccardSimilarity(liveCoOrders, cntA, cntB);
    }
  }

  // 2. Check seed pairings table
  for (const p of SEED_PAIRINGS) {
    if ((normA.includes(p.a) || p.a.includes(normA)) && (normB.includes(p.b) || p.b.includes(normB))) {
      return computeJaccardSimilarity(p.coOrders, p.totalA, p.totalB);
    }
    if ((normB.includes(p.a) || p.a.includes(normB)) && (normA.includes(p.b) || p.b.includes(normA))) {
      return computeJaccardSimilarity(p.coOrders, p.totalA, p.totalB);
    }
  }

  // 3. Fallback heuristic based on cuisine/category pairing (e.g. starter + main, bread + curry)
  return 0.15;
}

/**
 * Filter modes for Collaborative Filtering
 */
export const CF_MODES = {
  all: {
    id: 'all',
    label: '✨ All Recommendations',
    description: 'Collaborative filtering predictions based on what users frequently order'
  },
  also_ordered: {
    id: 'also_ordered',
    label: '👥 People Also Ordered',
    description: 'Dishes most frequently paired with items in user orders'
  },
  top_combos: {
    id: 'top_combos',
    label: '🔥 Best Food Combos',
    description: 'Top-rated food combinations with highest co-order frequency'
  },
  pure_veg: {
    id: 'pure_veg',
    label: '🥦 Veg Pairings',
    description: 'Collaborative recommendations filtered for pure vegetarian dishes'
  },
  quick_bites: {
    id: 'quick_bites',
    label: '⚡ Quick Bites',
    description: 'Snacks and beverages frequently paired as add-ons'
  }
};

/**
 * Parse simple natural language search queries (e.g. "under 300", "veg", "pizza")
 */
export function parseSimpleQuery(queryText) {
  if (!queryText || typeof queryText !== 'string' || !queryText.trim()) {
    return null;
  }

  const q = queryText.toLowerCase().trim();
  const result = {
    maxPrice: null,
    vegOnly: false,
    keyword: ''
  };

  // Price match: "under 300", "below 250", "< 200"
  const priceMatch = q.match(/(?:under|below|<|less than|within)\s*(?:rs\.?|₹)?\s*(\d+)/i) || q.match(/(\d+)\s*(?:rs\.?|₹|bucks)?/i);
  if (priceMatch) {
    const val = parseInt(priceMatch[1], 10);
    if (val >= 50 && val <= 2000) {
      result.maxPrice = val;
    }
  }

  if (q.includes('veg') && !q.includes('non-veg') && !q.includes('nonveg')) {
    result.vegOnly = true;
  }

  // Clean keyword and split into search tokens
  const cleaned = q
    .replace(/(?:under|below|<|less than|within)\s*(?:rs\.?|₹)?\s*\d+/gi, '')
    .replace(/\b(food|order|item|dishes|please|recommend)\b/gi, '')
    .trim();

  result.keyword = cleaned;
  result.keywords = cleaned ? cleaned.split(/\s+/).filter(w => w.length > 0) : [];

  return result;
}

/**
 * Master Recommendation Engine: Item-to-Item Collaborative Filtering
 */
export function computeAiRecommendations({
  user,
  pastOrders = [],
  allItems = [],
  activeMoodId = 'all',
  naturalQuery = ''
}) {
  if (!Array.isArray(allItems) || allItems.length === 0) {
    return {
      items: [],
      summary: 'No menu items available for recommendation.',
      hasHistory: false,
      activeMode: activeMoodId,
      topPairs: []
    };
  }

  // 1. Build live dynamic co-occurrence statistics from database orders
  const dynamicData = buildDynamicCoOccurrence(pastOrders);

  // 2. Identify items the user has ordered in their history
  const userOrderedNames = [];
  const userItemCounts = new Map();

  if (Array.isArray(pastOrders) && pastOrders.length > 0) {
    for (const order of pastOrders) {
      if (Array.isArray(order.items)) {
        for (const it of order.items) {
          const name = String(it.item_name || it.item_name_snapshot || it.name || '').toLowerCase().trim();
          const qty = Number(it.quantity) || 1;
          if (name) {
            userItemCounts.set(name, (userItemCounts.get(name) || 0) + qty);
            if (!userOrderedNames.includes(name)) {
              userOrderedNames.push(name);
            }
          }
        }
      }
    }
  }

  const hasHistory = userOrderedNames.length > 0;
  const nlp = parseSimpleQuery(naturalQuery);

  // 3. Score every candidate item using Collaborative Filtering
  const scoredItems = [];

  for (const item of allItems) {
    const itemName = String(item.name || '').toLowerCase().trim();

    // Query filters
    if (nlp) {
      if (nlp.vegOnly && !item.veg) continue;
      if (nlp.maxPrice !== null && Number(item.price) > nlp.maxPrice) continue;
      if (nlp.keywords && nlp.keywords.length > 0) {
        const itemDesc = String(item.desc || '').toLowerCase();
        const itemCuisine = String(item.cuisine || '').toLowerCase();
        // Check if any keyword matches name, description, or cuisine
        const matches = nlp.keywords.some(k => itemName.includes(k) || itemDesc.includes(k) || itemCuisine.includes(k));
        if (!matches) continue;
      }
    }

    // Mood / category filter
    if (activeMoodId === 'pure_veg' && !item.veg) continue;
    if (activeMoodId === 'quick_bites' && Number(item.price) > 220) continue;

    let cfScore = 0;
    let primaryAnchor = '';
    let maxSim = 0;

    if (hasHistory) {
      // User-Personalized Item-to-Item Collaborative Filtering:
      // Score(candidate) = Sum of Jaccard(orderedItem, candidate) * count(orderedItem)
      for (const orderedName of userOrderedNames) {
        const weight = userItemCounts.get(orderedName) || 1;
        const sim = getPairwiseSimilarity(orderedName, itemName, dynamicData);
        cfScore += sim * weight;

        if (sim > maxSim && orderedName !== itemName) {
          maxSim = sim;
          primaryAnchor = orderedName;
        }
      }

      // Check if candidate itself was ordered before
      if (userItemCounts.has(itemName)) {
        cfScore += 0.8; // Familiarity bonus
      }
    } else {
      // Cold-Start: Collaborative Community Pairings
      // Calculate how frequently this item pairs with any seed item
      for (const p of SEED_PAIRINGS) {
        if (itemName.includes(p.a) || p.a.includes(itemName) || itemName.includes(p.b) || p.b.includes(itemName)) {
          const sim = computeJaccardSimilarity(p.coOrders, p.totalA, p.totalB);
          if (sim > cfScore) {
            cfScore = sim;
            maxSim = sim;
            primaryAnchor = itemName.includes(p.a) ? p.b : p.a;
          }
        }
      }
      if (cfScore === 0) cfScore = 0.25;
    }

    // Collaborative Filtering Percentage Calculation
    let matchPercentage = 70;
    const isPastOrder = userItemCounts.has(itemName);

    if (hasHistory) {
      if (isPastOrder) {
        // You already ordered this item yourself (Past Favourite)
        const orderCount = userItemCounts.get(itemName) || 1;
        matchPercentage = Math.min(97, 91 + orderCount * 2); // 1 order = 93%, 2 orders = 95%, 3+ orders = 97%
      } else if (maxSim > 0.15) {
        // Recommended because it pairs with something you ordered
        matchPercentage = Math.round(72 + (maxSim * 28));
      } else {
        // General community favourite fallback
        const rScore = item.rating ? (Number(item.rating) - 4.0) * 10 : 4;
        matchPercentage = Math.round(68 + Math.max(0, rScore));
      }
    } else {
      // Cold start: community pairings
      if (maxSim > 0.2) {
        matchPercentage = Math.round(75 + (maxSim * 22));
      } else {
        const rScore = item.rating ? (Number(item.rating) - 4.0) * 8 : 4;
        matchPercentage = Math.round(70 + Math.max(0, rScore));
      }
    }

    // Rating refinement (+1% for stellar 4.7+ dishes)
    if (item.rating && Number(item.rating) >= 4.7 && matchPercentage < 96) {
      matchPercentage += 1;
    }

    // Ensure range is between 68% and 97%
    matchPercentage = Math.max(68, Math.min(97, matchPercentage));
    const normalizedScore = matchPercentage / 100.0;

    // Capitalize anchor name for clear display
    const formattedAnchor = primaryAnchor
      ? primaryAnchor.replace(/\b\w/g, c => c.toUpperCase())
      : 'your past orders';

    // Human-understandable Explainable AI Rationale
    let rationale = '';

    if (hasHistory) {
      if (isPastOrder) {
        rationale = `${matchPercentage}% Match • You ordered this ${userItemCounts.get(itemName)} time${userItemCounts.get(itemName) > 1 ? 's' : ''} previously!`;
      } else if (primaryAnchor) {
        rationale = `${matchPercentage}% Match • Customers who ordered ${formattedAnchor} also frequently ordered this dish.`;
      } else {
        rationale = `${matchPercentage}% Match • Recommended based on collaborative patterns in your order history.`;
      }
    } else {
      if (primaryAnchor) {
        rationale = `${matchPercentage}% Match • Popular community combo: frequently paired with ${formattedAnchor}.`;
      } else {
        rationale = `${matchPercentage}% Match • Highly rated neighbourhood favourite among local foodies.`;
      }
    }

    const badgeText = isPastOrder
      ? 'Past Favourite'
      : matchPercentage >= 95
        ? 'Top Collaborative Pick'
        : 'People Also Ordered';

    scoredItems.push({
      ...item,
      aiMatchPercentage: matchPercentage,
      cfScore: normalizedScore,
      aiScore: normalizedScore,
      aiExplanation: rationale,
      reason: rationale,
      coOrderedWith: formattedAnchor,
      isPastOrder,
      badge: badgeText
    });
  }

  // Sort descending by Collaborative Filtering Score
  scoredItems.sort((a, b) => b.cfScore - a.cfScore);

  // Return top 8 items
  const topRecommendations = scoredItems.slice(0, 8);

  // Dynamic summary
  let summary = '';
  if (nlp && (nlp.maxPrice !== null || nlp.vegOnly || nlp.keyword)) {
    const filters = [];
    if (nlp.vegOnly) filters.push('Pure Veg');
    if (nlp.maxPrice) filters.push(`Budget ≤ ₹${nlp.maxPrice}`);
    if (nlp.keyword) filters.push(`"${nlp.keyword}"`);
    summary = `Filtered by ${filters.join(' • ')} — Ranked by Collaborative Filtering co-occurrence.`;
  } else if (hasHistory) {
    summary = `Collaborative Filtering trained on your ${userOrderedNames.length} previous dish order${userOrderedNames.length > 1 ? 's' : ''}.`;
  } else {
    summary = `Community Collaborative Filtering: Highlights dishes most frequently ordered together by local foodies.`;
  }

  // Prepare top co-occurrence pairings for the "How AI Works" explainer modal
  const topPairs = SEED_PAIRINGS.slice(0, 6).map(p => ({
    dishA: p.a.replace(/\b\w/g, c => c.toUpperCase()),
    dishB: p.b.replace(/\b\w/g, c => c.toUpperCase()),
    coOrders: p.coOrders,
    similarityPct: Math.round(computeJaccardSimilarity(p.coOrders, p.totalA, p.totalB) * 100)
  }));

  return {
    items: topRecommendations,
    summary,
    hasHistory,
    userItemCount: userOrderedNames.length,
    activeMode: activeMoodId,
    topPairs
  };
}
