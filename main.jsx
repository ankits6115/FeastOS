import React, { useEffect, useMemo, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { MapContainer, TileLayer, Marker, Popup, ZoomControl, useMap, useMapEvents, CircleMarker, Polyline } from 'react-leaflet';
import L from 'leaflet';
import { Search, ShoppingBag, MapPin, Clock3, Star, SlidersHorizontal, Plus, Minus, X, ArrowRight, Heart, Navigation, Bike, Check, ChevronDown, LocateFixed, RefreshCw, ClipboardList, BarChart3, User, Mail, Lock, LogOut, Eye, EyeOff, Sparkles, RotateCcw, Flame } from 'lucide-react';
import 'leaflet/dist/leaflet.css';
import './styles.css';

const demoRestaurants = [
 {id:1,name:'Toscano Wood-fired Pizza',cuisine:'Italian · Pizza · Pasta',rating:4.8,time:'20-30 min',fee:'₹25 delivery',offer:'20% OFF up to ₹120',tag:'Popular near you',lat:12.9352,lng:77.6245,hero:'https://images.unsplash.com/photo-1579751626657-72bc17010498?auto=format&fit=crop&w=900&q=85',items:[{id:'p1',name:'Margherita Pizza',desc:'San Marzano tomato, mozzarella, fresh basil',price:299,image:'https://images.unsplash.com/photo-1579751626657-72bc17010498?auto=format&fit=crop&w=700&q=85',veg:true},{id:'p2',name:'Truffle Mushroom Pizza',desc:'Roasted mushrooms, truffle cream, parmesan',price:429,image:'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=700&q=85',veg:true},{id:'p3',name:'Penne Arrabbiata',desc:'Penne in a spicy tomato and garlic sauce',price:279,image:'https://images.unsplash.com/photo-1473093295043-cdd812d0e601?auto=format&fit=crop&w=700&q=85',veg:true}]},
 {id:2,name:'Saffron & Spice',cuisine:'North Indian · Biryani · Curry',rating:4.6,time:'25-35 min',fee:'₹19 delivery',offer:'Free dessert above ₹499',tag:'Bestseller',lat:12.9716,lng:77.5946,hero:'https://images.unsplash.com/photo-1585937421612-70a008356fbe?auto=format&fit=crop&w=900&q=85',items:[{id:'i1',name:'Paneer Butter Masala',desc:'Creamy tomato gravy, house spices',price:289,image:'https://images.unsplash.com/photo-1631452180519-c014fe946bc7?auto=format&fit=crop&w=700&q=85',veg:true},{id:'i2',name:'Hyderabadi Dum Biryani',desc:'Fragrant basmati rice, herbs and spices',price:329,image:'https://images.unsplash.com/photo-1563379091339-03246963d96c?auto=format&fit=crop&w=700&q=85',veg:false},{id:'i3',name:'Butter Naan (2 pcs)',desc:'Tandoor-baked, brushed with butter',price:79,image:'https://images.unsplash.com/photo-1601050690597-df0568f70950?auto=format&fit=crop&w=700&q=85',veg:true}]},
 {id:3,name:'Green Bowl Kitchen',cuisine:'Healthy · Bowls · Salads',rating:4.7,time:'15-25 min',fee:'₹15 delivery',offer:'15% OFF your first order',tag:'Fresh & light',lat:12.945,lng:77.61,hero:'https://images.unsplash.com/photo-1512621776951-a57141f2eefd?auto=format&fit=crop&w=900&q=85',items:[{id:'g1',name:'Avocado Power Bowl',desc:'Avocado, quinoa, greens, citrus dressing',price:319,image:'https://images.unsplash.com/photo-1512621776951-a57141f2eefd?auto=format&fit=crop&w=700&q=85',veg:true},{id:'g2',name:'Crispy Tofu Salad',desc:'Sesame tofu, crunchy vegetables, ginger lime',price:269,image:'https://images.unsplash.com/photo-1540420773420-3366772f4999?auto=format&fit=crop&w=700&q=85',veg:true},{id:'g3',name:'Berry Yogurt Bowl',desc:'Greek yogurt, berries, granola and honey',price:229,image:'https://images.unsplash.com/photo-1490474418585-ba9bad8fd0ea?auto=format&fit=crop&w=700&q=85',veg:true}]},
 {id:4,name:'The French Baker',cuisine:'Bakery · Desserts · Coffee',rating:4.9,time:'20-30 min',fee:'₹20 delivery',offer:'Buy 2 pastries, get 1 free',tag:'Sweet treats',lat:12.96,lng:77.638,hero:'https://images.unsplash.com/photo-1509440159596-0249088772ff?auto=format&fit=crop&w=900&q=85',items:[{id:'f1',name:'Butter Croissant',desc:'Flaky, golden, baked fresh this morning',price:119,image:'https://images.unsplash.com/photo-1555507036-ab1f4038808a?auto=format&fit=crop&w=700&q=85',veg:true},{id:'f2',name:'Chocolate Lava Cake',desc:'Warm chocolate cake with a molten centre',price:189,image:'https://images.unsplash.com/photo-1606313564200-e75d5e30476c?auto=format&fit=crop&w=700&q=85',veg:true},{id:'f3',name:'Iced Vanilla Latte',desc:'Double espresso, milk and vanilla',price:169,image:'https://images.unsplash.com/photo-1461023058943-07fcbe16d735?auto=format&fit=crop&w=700&q=85',veg:true}]},
 {id:5,name:'Milano Gelato Parlour',cuisine:'Gelato · Ice Cream · Desserts',rating:4.8,time:'15-20 min',fee:'₹15 delivery',offer:'Free topping with any 2 scoops',tag:'Dessert favourite',lat:12.925,lng:77.606,hero:'https://images.unsplash.com/photo-1563805042-7684c019e1cb?auto=format&fit=crop&w=900&q=85',items:[{id:'m1',name:'Pistachio Gelato',desc:'Slow-churned Sicilian-style pistachio',price:179,image:'https://images.unsplash.com/photo-1563805042-7684c019e1cb?auto=format&fit=crop&w=700&q=85',veg:true},{id:'m2',name:'Chocolate Sundae',desc:'Chocolate gelato, fudge and crisp flakes',price:219,image:'https://images.unsplash.com/photo-1560008581-09826d1de69e?auto=format&fit=crop&w=700&q=85',veg:true}]},
 {id:6,name:'Sizzler Town',cuisine:'Continental · Sizzlers · Grills',rating:4.5,time:'30-40 min',fee:'₹29 delivery',offer:'₹100 OFF above ₹599',tag:'Comfort food',lat:12.955,lng:77.59,hero:'https://images.unsplash.com/photo-1544025162-d76694265947?auto=format&fit=crop&w=900&q=85',items:[{id:'s1',name:'Grilled Paneer Sizzler',desc:'Smoky paneer, vegetables, pepper sauce',price:349,image:'https://images.unsplash.com/photo-1544025162-d76694265947?auto=format&fit=crop&w=700&q=85',veg:true},{id:'s2',name:'Classic Chicken Grill',desc:'Herb grilled chicken with seasonal sides',price:429,image:'https://images.unsplash.com/photo-1532550907401-a500c9a57435?auto=format&fit=crop&w=700&q=85',veg:false}]}
];
const categories=[['All','✨'],['Pizza','🍕'],['Indian','🍛'],['Healthy','🥗'],['Bakery','🥐'],['Desserts','🍨'],['Grills','🍗']];
const FOOD_FALLBACKS={
 pizza:'https://images.unsplash.com/photo-1513104890138-7c749659a591?auto=format&fit=crop&w=700&q=85',
 burger:'https://images.unsplash.com/photo-1568901346375-23c9450c58cd?auto=format&fit=crop&w=700&q=85',
 fry:'https://images.unsplash.com/photo-1573080496219-bb080dd4f877?auto=format&fit=crop&w=700&q=85',
 fries:'https://images.unsplash.com/photo-1573080496219-bb080dd4f877?auto=format&fit=crop&w=700&q=85',
 'paneer tikka':'https://images.unsplash.com/photo-1567188040759-fb8a883dc6d8?auto=format&fit=crop&w=700&q=85',
 paneer:'https://images.unsplash.com/photo-1631452180519-c014fe946bc7?auto=format&fit=crop&w=700&q=85',
 tikka:'https://images.unsplash.com/photo-1567188040759-fb8a883dc6d8?auto=format&fit=crop&w=700&q=85',
 curry:'https://images.unsplash.com/photo-1588166524941-3bf61a9c41db?auto=format&fit=crop&w=700&q=85',
 dal:'https://images.unsplash.com/photo-1546833999-b9f581a1996d?auto=format&fit=crop&w=700&q=85',
 biryani:'https://images.unsplash.com/photo-1589302168068-964664d93dc0?auto=format&fit=crop&w=700&q=85',
 rice:'https://images.unsplash.com/photo-1603133872878-684f208fb84b?auto=format&fit=crop&w=700&q=85',
 noodle:'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=700&q=85',
 chowmein:'https://images.unsplash.com/photo-1569718212165-3a8278d5f624?auto=format&fit=crop&w=700&q=85',
 momo:'https://images.unsplash.com/photo-1534422298391-e4f8c172dddb?auto=format&fit=crop&w=700&q=85',
 coffee:'https://images.unsplash.com/photo-1534778101976-62847782c213?auto=format&fit=crop&w=700&q=85',
 latte:'https://images.unsplash.com/photo-1534778101976-62847782c213?auto=format&fit=crop&w=700&q=85',
 cappuccino:'https://images.unsplash.com/photo-1572442388796-11668a67e53d?auto=format&fit=crop&w=700&q=85',
 taco:'https://images.unsplash.com/photo-1551504734-5ee1c4a1479b?auto=format&fit=crop&w=700&q=85',
 burrito:'https://images.unsplash.com/photo-1626700051175-6818013e1d4f?auto=format&fit=crop&w=700&q=85',
 nacho:'https://images.unsplash.com/photo-1513456852971-30c0b8199d4d?auto=format&fit=crop&w=700&q=85',
 pasta:'https://images.unsplash.com/photo-1473093295043-cdd812d0e601?auto=format&fit=crop&w=700&q=85',
 chicken:'https://images.unsplash.com/photo-1567620832903-9fc6debc209f?auto=format&fit=crop&w=700&q=85',
 sandwich:'https://images.unsplash.com/photo-1528735602780-2552fd46c7af?auto=format&fit=crop&w=700&q=85',
 sub:'https://images.unsplash.com/photo-1509722747041-616f39b57569?auto=format&fit=crop&w=700&q=85',
 brownie:'https://images.unsplash.com/photo-1606313564200-e75d5e30476c?auto=format&fit=crop&w=700&q=85'
};
function getFoodFallback(name,isVeg){
 const lower=String(name||'').toLowerCase();
 for(const [k,url] of Object.entries(FOOD_FALLBACKS)){
  if(lower.includes(k)) return url;
 }
 return isVeg?'https://images.unsplash.com/photo-1540420773420-3366772f4999?auto=format&fit=crop&w=700&q=85':'https://images.unsplash.com/photo-1544025162-d76694265947?auto=format&fit=crop&w=700&q=85';
}
const money=n=>'₹'+Number(n||0).toLocaleString('en-IN');
function formatDateTime(value){
 if(value===null||value===undefined||value==='') return 'Time unavailable';
 const date = new Date(value);
 if(Number.isNaN(date.getTime())) return 'Time unavailable';
 return date.toLocaleString(undefined,{year:'numeric',month:'short',day:'numeric',hour:'2-digit',minute:'2-digit'});
}
const pinIcon=L.divIcon({className:'restaurant-pin',html:'<span>🍴</span>',iconSize:[34,34],iconAnchor:[17,17]});
const deliveryPinIcon=L.divIcon({className:'delivery-pin',html:'<span>⌂</span>',iconSize:[36,36],iconAnchor:[18,32]});
function haversineKm(lat1,lon1,lat2,lon2){const rad=n=>n*Math.PI/180;const dLat=rad(lat2-lat1),dLon=rad(lon2-lon1);const a=Math.sin(dLat/2)**2+Math.cos(rad(lat1))*Math.cos(rad(lat2))*Math.sin(dLon/2)**2;return 6371*2*Math.atan2(Math.sqrt(a),Math.sqrt(Math.max(0,1-a)));}
function MapClickHandler({enabled,onPick}){useMapEvents({click(e){if(enabled)onPick({lat:e.latlng.lat,lng:e.latlng.lng,label:`Pinned map location (${e.latlng.lat.toFixed(5)}, ${e.latlng.lng.toFixed(5)})`});}});return null;}
function Recenter({center}){const map=useMap();useEffect(()=>{if(center&&center.every(Number.isFinite))map.flyTo(center,14,{duration:0.8});},[center,map]);return null;}
function FitRestaurants({restaurants,fitKey}){const map=useMap();useEffect(()=>{const points=restaurants.filter(r=>Number.isFinite(Number(r.lat))&&Number.isFinite(Number(r.lng))).map(r=>[Number(r.lat),Number(r.lng)]);if(points.length>1)map.fitBounds(L.latLngBounds(points),{padding:[28,28],maxZoom:14});else if(points.length===1)map.setView(points[0],14);},[restaurants,fitKey,map]);return null;}

function App(){
 const [restaurants,setRestaurants]=useState(demoRestaurants),[backendStatus,setBackendStatus]=useState('connecting');
 const locationInputRef=useRef(null);
 const userMenuRef=useRef(null);
 const [user,setUser]=useState(()=>{
  try{const saved=localStorage.getItem('feastos_user');return saved?JSON.parse(saved):null;}catch{return null;}
 });
 const [userMenuOpen,setUserMenuOpen]=useState(false);
 const [authOpen,setAuthOpen]=useState(false);
 const [authTab,setAuthTab]=useState('continue'); // 'continue' | 'login' | 'signup'
 const [authStep,setAuthStep]=useState('email'); // 'email' | 'login' | 'signup'
 const [authEmail,setAuthEmail]=useState('');
 const [authPassword,setAuthPassword]=useState('');
 const [authName,setAuthName]=useState('');
 const [showPassword,setShowPassword]=useState(false);
 const [authLoading,setAuthLoading]=useState(false);
 const [authError,setAuthError]=useState('');
 const [category,setCategory]=useState('All'),[query,setQuery]=useState(''),[selected,setSelected]=useState(null),[cart,setCart]=useState({}),[cartOpen,setCartOpen]=useState(false),[liked,setLiked]=useState([]),[toast,setToast]=useState(''),[sort,setSort]=useState('Recommended');
 const [ordersOpen,setOrdersOpen]=useState(false),[pastOrders,setPastOrders]=useState([]),[ordersLoading,setOrdersLoading]=useState(false),[ordersError,setOrdersError]=useState('');
 const [recommendations,setRecommendations]=useState(null),[recLoading,setRecLoading]=useState(false);
 const [schedulingStats,setSchedulingStats]=useState(null),[schedulingLoading,setSchedulingLoading]=useState(false),[schedulingError,setSchedulingError]=useState('');
 const [center,setCenter]=useState([12.955,77.615]),[userLocation,setUserLocation]=useState(null),[deliveryLocation,setDeliveryLocation]=useState(null),[locationQuery,setLocationQuery]=useState(''),[locationResults,setLocationResults]=useState([]),[locationSearchBusy,setLocationSearchBusy]=useState(false),[pickDeliveryMode,setPickDeliveryMode]=useState(false),[routeCoordinates,setRouteCoordinates]=useState([]),[routeInfo,setRouteInfo]=useState(null),[routeBusy,setRouteBusy]=useState(false),[fitKey,setFitKey]=useState(0);
 useEffect(()=>{
  if(!userMenuOpen)return;
  function handleDocClick(e){if(userMenuRef.current&&!userMenuRef.current.contains(e.target))setUserMenuOpen(false);}
  document.addEventListener('mousedown',handleDocClick);
  return()=>document.removeEventListener('mousedown',handleDocClick);
 },[userMenuOpen]);
 useEffect(()=>{
  if(!ordersOpen&&!authOpen)return;
  const previousOverflow=document.body.style.overflow;
  document.body.style.overflow='hidden';
  return()=>{document.body.style.overflow=previousOverflow;};
 },[ordersOpen,authOpen]);
 useEffect(()=>{let active=true;(async()=>{try{const response=await fetch('http://127.0.0.1:8787/api/restaurants');if(!response.ok)throw new Error('API unavailable');const data=await response.json();if(!Array.isArray(data))throw new Error('Unexpected API response');if(active){setRestaurants(data);setBackendStatus('connected');}}catch{if(active)setBackendStatus('offline');}})();return()=>{active=false;}},[]);
 const items=useMemo(()=>restaurants.flatMap(r=>(r.items||[]).map(i=>({...i,restaurantId:r.id,restaurantName:r.name}))),[restaurants]);
 const allAvailableItems=useMemo(()=>{
  const map=new Map();
  for(const i of items) map.set(String(i.id),i);
  if(recommendations&&Array.isArray(recommendations.items)){
   for(const i of recommendations.items){
    if(!map.has(String(i.id))) map.set(String(i.id),i);
   }
  }
  return Array.from(map.values());
 },[items,recommendations]);
 const cartRows=allAvailableItems.filter(i=>cart[String(i.id)]>0),count=cartRows.reduce((n,i)=>n+(cart[String(i.id)]||0),0),subtotal=cartRows.reduce((n,i)=>n+i.price*(cart[String(i.id)]||0),0);
 const visible=restaurants.filter(r=>(category==='All'||String(r.cuisine||'').toLowerCase().includes(category.toLowerCase())||(category==='Pizza'&&String(r.cuisine||'').includes('Italian'))||(category==='Indian'&&String(r.cuisine||'').includes('North Indian'))||(category==='Grills'&&String(r.cuisine||'').includes('Grills')))&&(`${r.name} ${r.cuisine} ${(r.items||[]).map(i=>i.name).join(' ')}`.toLowerCase().includes(query.toLowerCase())));
 const ordered=[...visible].sort((a,b)=>sort==='Rating'?Number(b.rating)-Number(a.rating):sort==='Fastest'?parseInt(a.time)-parseInt(b.time):String(a.name).localeCompare(String(b.name)));
 function notify(s){setToast(s);setTimeout(()=>setToast(''),4200);}
 function change(id,delta){const sid=String(id);setCart(prev=>{const next={...prev},v=(next[sid]||0)+delta;if(v<=0)delete next[sid];else next[sid]=v;return next;});}
 function fallbackRecommendations(){
  if(!items||items.length===0)return;
  const topItems=items.slice(0,6).map(it=>({...it,badge:'Chef Pick',reason:'Top favourite in your neighbourhood'}));
  setRecommendations({
   title:'Recommended For You',
   subtitle:'Popular dishes loved across your area',
   source:'top_rated',
   items:topItems
  });
 }
 async function loadRecommendations(userEmail){
  setRecLoading(true);
  try{
   const email=(typeof userEmail==='string'?userEmail:(user?.email||'')).trim();
   let res;
   if(email){
    res=await fetch('http://127.0.0.1:8787/api/recommendations',{
     method:'POST',
     headers:{'Content-Type':'application/json'},
     body:JSON.stringify({email})
    });
   }else{
    res=await fetch('http://127.0.0.1:8787/api/recommendations');
   }
   if(!res.ok)throw new Error('API unavailable');
   const data=await res.json();
   if(data&&Array.isArray(data.items)&&data.items.length>0){
    setRecommendations(data);
   }else{
    fallbackRecommendations();
   }
  }catch{
   fallbackRecommendations();
  }finally{
   setRecLoading(false);
  }
 }
 function handleReorder(order){
  if(!Array.isArray(order.items)||order.items.length===0){
   notify('No items found in this order to reorder.');
   return;
  }
  const newCart={};
  let addedCount=0;
  for(const it of order.items){
   const itemId=String(it.menu_item_id||it.item_id||it.id||'');
   const qty=Number(it.quantity)||1;
   if(itemId){
    newCart[itemId]=(newCart[itemId]||0)+qty;
    addedCount+=qty;
   }
  }
  if(addedCount>0){
   setCart(newCart);
   setOrdersOpen(false);
   setCartOpen(true);
   notify(`Reordering ${addedCount} item${addedCount>1?'s':''} from Order #${order.order_id??order.id}`);
  }else{
   notify('Could not add items to cart.');
  }
 }
 useEffect(()=>{
  loadRecommendations(user?.email);
 },[user,backendStatus]);
 async function loadPastOrders(){setOrdersOpen(true);setOrdersLoading(true);setOrdersError('');loadSchedulingStats();try{const response=await fetch('http://127.0.0.1:8787/api/orders/history');const data=await response.json();if(!response.ok)throw new Error(data.error||'Could not load past orders.');const list=Array.isArray(data)?data:data.orders;if(!Array.isArray(list))throw new Error('Unexpected order-history API response.');setPastOrders(list);}catch(error){setOrdersError(error.message==='Failed to fetch'?'Could not connect to the FeastOS API. Check that the backend is running.':error.message);}finally{setOrdersLoading(false);}}
 async function loadSchedulingStats(){setSchedulingLoading(true);setSchedulingError('');try{const response=await fetch('http://127.0.0.1:8787/api/orders/scheduling-stats');const data=await response.json();if(!response.ok)throw new Error(data.error||'Scheduling statistics are unavailable.');setSchedulingStats(data);}catch(error){setSchedulingStats(null);setSchedulingError(error.message==='Failed to fetch'?'Could not connect to the scheduling API.':error.message);}finally{setSchedulingLoading(false);}}
 // Refresh order and dispatch status while the Orders drawer is open. The API can also finalize deliveries whose estimated time has elapsed.
 useEffect(()=>{if(!ordersOpen)return;let active=true;const refresh=async()=>{try{const response=await fetch('http://127.0.0.1:8787/api/orders/history');if(!response.ok)return;const data=await response.json();const list=Array.isArray(data)?data:data.orders;if(active&&Array.isArray(list))setPastOrders(list);}catch{/* Keep the last successfully loaded order list visible. */}};const timer=setInterval(refresh,5000);return()=>{active=false;clearInterval(timer);};},[ordersOpen]);
 function handleLogout(){
  setUser(null);
  localStorage.removeItem('feastos_user');
  setUserMenuOpen(false);
  loadRecommendations('');
  notify('You have been logged out.');
 }

 async function handleAuthSubmit(e){
  e.preventDefault();
  setAuthError('');
  const email=authEmail.trim().toLowerCase();
  if(!email||!email.includes('@')||!email.includes('.')){
   setAuthError('Please enter a valid email address.');
   return;
  }
  if(authTab==='continue'&&authStep==='email'){
   setAuthLoading(true);
   try{
    const res=await fetch('http://127.0.0.1:8787/api/auth/check-email',{
     method:'POST',
     headers:{'Content-Type':'application/json'},
     body:JSON.stringify({email})
    });
    const data=await res.json();
    if(data.exists){
     setAuthStep('login');
     if(data.name)setAuthName(data.name);
    }else{
     setAuthStep('signup');
    }
   }catch{
    setAuthStep('signup');
   }finally{
    setAuthLoading(false);
   }
   return;
  }
  const isSignup=authTab==='signup'||(authTab==='continue'&&authStep==='signup');
  if(!authPassword||authPassword.length<4){
   setAuthError('Password must be at least 4 characters long.');
   return;
  }
  setAuthLoading(true);
  try{
   const endpoint=isSignup?'http://127.0.0.1:8787/api/auth/signup':'http://127.0.0.1:8787/api/auth/login';
   const payload={
    email,
    password:authPassword,
    ...(isSignup?{name:authName.trim()||email.split('@')[0]}:{})
   };
   const res=await fetch(endpoint,{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify(payload)
   });
   const data=await res.json();
   if(!res.ok||!data.ok){
    throw new Error(data.error||'Authentication failed. Please verify your credentials.');
   }
   setUser(data.user);
   localStorage.setItem('feastos_user',JSON.stringify(data.user));
   setAuthOpen(false);
   setAuthPassword('');
   setAuthStep('email');
   loadRecommendations(data.user.email);
   notify(isSignup?`Welcome to FeastOS, ${data.user.name}!`:`Welcome back, ${data.user.name}!`);
  }catch(err){
   setAuthError(err.message==='Failed to fetch'?'Cannot connect to FeastOS API. Make sure the backend server is running.':err.message);
  }finally{
   setAuthLoading(false);
  }
 }

 async function placeRazorpayOrder(){
  if(!deliveryLocation){notify('Choose a delivery location first.');return;}
  if(new Set(cartRows.map(i=>String(i.restaurantId))).size>1){notify('One order can contain items from one restaurant only.');return;}
  if(backendStatus!=='connected'||!cartRows.length){notify('Connect FeastOS and add at least one database menu item.');return;}
  const payload={items:cartRows.map(item=>({item_id:Number(item.id),quantity:cart[item.id]})),latitude:deliveryLocation.lat,longitude:deliveryLocation.lng,label:deliveryLocation.label,user_email:user?.email||''};
  if(payload.items.some(item=>!Number.isSafeInteger(item.item_id)||item.item_id<=0)){notify('Refresh the database menu and add valid items.');return;}
  const loadCheckout=()=>new Promise((resolve,reject)=>{if(window.Razorpay){resolve();return;}const script=document.createElement('script');script.src='https://checkout.razorpay.com/v1/checkout.js';script.onload=resolve;script.onerror=()=>reject(new Error('Could not load Razorpay Checkout'));document.body.appendChild(script);});
  try{
   const createdResponse=await fetch('http://127.0.0.1:8787/api/orders/razorpay/create',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
   const created=await createdResponse.json();if(!createdResponse.ok)throw new Error(created.error||'Could not create Razorpay order');
   await loadCheckout();
   const options={key:created.key_id,amount:created.amount,currency:created.currency||'INR',name:'FeastOS',description:`Order #${created.order_id}`,order_id:created.razorpay_order_id,handler:async(payment)=>{
     try{const verifiedResponse=await fetch('http://127.0.0.1:8787/api/payments/razorpay/verify',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({razorpay_order_id:payment.razorpay_order_id,razorpay_payment_id:payment.razorpay_payment_id,razorpay_signature:payment.razorpay_signature})});const verified=await verifiedResponse.json();if(!verifiedResponse.ok||verified.payment_status!=='paid')throw new Error(verified.error||'Payment verification failed');
      setCart({});setCartOpen(false);setSchedulingStats(null);notify(`Payment verified · FeastOS order #${created.order_id} confirmed`);await loadSchedulingStats();loadRecommendations(user?.email);
     }catch(error){notify(`Payment received but FeastOS could not verify it yet. Keep your payment ID: ${payment.razorpay_payment_id}. ${error.message}`);}
    },modal:{ondismiss:()=>notify(`Checkout closed. Order #${created.order_id} remains unpaid until verified.`)},prefill:{},theme:{color:'#b85c38'}};
   const checkout=new window.Razorpay(options);checkout.on('payment.failed',event=>notify(event.error?.description||'Payment failed. Your order remains unpaid.'));checkout.open();
  }catch(error){notify(`Payment not started: ${error.message}`);}
 }

 async function placeCodOrder(){
  if(!deliveryLocation){notify('Choose a delivery location first.');return;}
  if(new Set(cartRows.map(i=>String(i.restaurantId))).size>1){notify('One order can contain items from one restaurant only.');return;}
  if(backendStatus!=='connected'||!cartRows.length){notify('Connect FeastOS and add at least one database menu item.');return;}
  const payload={items:cartRows.map(item=>({item_id:Number(item.id),quantity:cart[item.id]})),latitude:deliveryLocation.lat,longitude:deliveryLocation.lng,label:deliveryLocation.label,user_email:user?.email||''};
  if(payload.items.some(item=>!Number.isSafeInteger(item.item_id)||item.item_id<=0)){notify('Refresh the database menu and add valid items.');return;}
  try{
   const response=await fetch('http://127.0.0.1:8787/api/orders/cod/create',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
   const created=await response.json();
   if(!response.ok||!created.ok)throw new Error(created.error||'Could not place cash-on-delivery order');
   setCart({});setCartOpen(false);setSchedulingStats(null);
   notify(`Order placed · FeastOS order #${created.order_id} · Pay cash on delivery`);
   await loadSchedulingStats();
   await loadPastOrders();
   loadRecommendations(user?.email);
  }catch(error){notify(`Order not placed: ${error.message}`);}
 }

 function focusRestaurant(r){setSelected(r);setCenter([Number(r.lat),Number(r.lng)]);setRouteCoordinates([]);setRouteInfo(null);}
 async function refreshRestaurants(){try{const response=await fetch('http://127.0.0.1:8787/api/restaurants');if(!response.ok)throw new Error('API unavailable');const data=await response.json();if(!Array.isArray(data))throw new Error('Unexpected API response');setRestaurants(data);setBackendStatus('connected');notify(`Restaurant data refreshed · ${data.length} restaurants`);}catch{setBackendStatus('offline');notify('Could not refresh from PostgreSQL. Check the C API connection.');}}
 function applyDeliveryLocation(location){const lat=Number(location.lat),lng=Number(location.lng);if(!Number.isFinite(lat)||!Number.isFinite(lng)||lat < -90||lat > 90||lng < -180||lng > 180){notify('That location has invalid coordinates.');return;}setDeliveryLocation({lat,lng,label:location.label||`Pinned location (${lat.toFixed(5)}, ${lng.toFixed(5)})`});setCenter([lat,lng]);setPickDeliveryMode(false);setLocationResults([]);setRouteCoordinates([]);setRouteInfo(null);notify('Delivery location selected.');}
 async function searchPlaces(event){event?.preventDefault();const q=locationQuery.trim();if(q.length<3){notify('Enter at least 3 characters to search for a place.');return;}if(locationSearchBusy)return;setLocationSearchBusy(true);setLocationResults([]);try{const url=`https://nominatim.openstreetmap.org/search?format=jsonv2&addressdetails=1&limit=6&countrycodes=in&q=${encodeURIComponent(q)}`;const response=await fetch(url,{headers:{Accept:'application/json'}});if(!response.ok)throw new Error('Geocoding service unavailable');const data=await response.json();setLocationResults(data.map(place=>({id:place.place_id,lat:Number(place.lat),lng:Number(place.lon),label:place.display_name})));if(!data.length)notify('No matching places found. Try a nearby landmark or full address.');}catch{notify('Place search is temporarily unavailable. You can choose a point directly on the map.');}finally{setLocationSearchBusy(false);}}
 function locateMe(){if(!navigator.geolocation){notify('Location is not available in this environment.');return;}notify('Requesting your location…');navigator.geolocation.getCurrentPosition(pos=>{const lat=pos.coords.latitude,lng=pos.coords.longitude;setUserLocation([lat,lng]);applyDeliveryLocation({lat,lng,label:'Current device location'});},err=>notify(err.code===1?'Location permission was denied. Enable location permission and try again.':err.code===2?'Your location could not be determined.':'Location request timed out. Try again.'),{enableHighAccuracy:true,timeout:12000,maximumAge:60000});}
 async function showRoute(){if(!selected){notify('Choose a restaurant first.');return;}if(!deliveryLocation){notify('Search for or choose a delivery location first.');return;}setRouteBusy(true);try{const fromLat=deliveryLocation.lat,fromLng=deliveryLocation.lng,toLat=Number(selected.lat),toLng=Number(selected.lng);const url=`https://router.project-osrm.org/route/v1/driving/${fromLng},${fromLat};${toLng},${toLat}?overview=full&geometries=geojson`;const response=await fetch(url);const data=await response.json();if(!response.ok||data.code!=='Ok'||!data.routes?.length)throw new Error('No route was returned');const route=data.routes[0];setRouteCoordinates(route.geometry.coordinates.map(([lng,lat])=>[lat,lng]));setRouteInfo({distance:(route.distance/1000).toFixed(1),duration:Math.max(1,Math.round(route.duration/60)),restaurantId:String(selected.id)});notify('Road route calculated from the delivery location.');}catch{notify('Could not load a road route right now. Try again later.');}finally{setRouteBusy(false);}}
 function fitAllRestaurants(){setSelected(null);setRouteCoordinates([]);setRouteInfo(null);if(restaurants.length){setFitKey(k=>k+1);notify('Showing all restaurant pins.');}else notify('No restaurant coordinates are available yet.');}

 return <div className="app">
  <header className="topbar"><div className="brand" onClick={()=>{setSelected(null);setCategory('All');setQuery('');}}><span className="brand-mark">f</span><span>feast<span className="brand-dark">OS</span><small>We Deliver Fastttt!!!!</small></span></div>
   <button className="location-button" onClick={()=>locationInputRef.current?.focus()}><MapPin size={17}/><span><b>Delivering to</b><strong>{deliveryLocation?(deliveryLocation.label.length>32?deliveryLocation.label.slice(0,29)+'…':deliveryLocation.label):'Choose your location'}</strong></span><ChevronDown size={15}/></button>
   <div className="searchbox"><Search size={19}/><input value={query} onChange={e=>setQuery(e.target.value)} placeholder="Search dishes, restaurants, cuisines..."/>{query&&<button className="icon-button" onClick={()=>setQuery('')}><X size={16}/></button>}</div>
   <button className="orders-link" onClick={loadPastOrders}><ClipboardList size={16}/> Orders</button>
   {user ? (
    <div className="user-menu-wrap" ref={userMenuRef}>
     <button className="user-button" onClick={()=>setUserMenuOpen(v=>!v)}>
      <span className="user-avatar-badge">{user.name ? user.name[0].toUpperCase() : 'U'}</span>
      <span className="user-name-text">{user.name || user.email.split('@')[0]}</span>
      <ChevronDown size={14}/>
     </button>
     {userMenuOpen && (
      <div className="user-dropdown">
       <div className="user-dropdown-info">
        <strong>{user.name || 'FeastOS Foodie'}</strong>
        <span>{user.email}</span>
       </div>
       <button className="user-dropdown-item" onClick={()=>{setUserMenuOpen(false); loadPastOrders();}}>
        <ClipboardList size={15}/> My Orders
       </button>
       <button className="user-dropdown-item danger" onClick={handleLogout}>
        <LogOut size={15}/> Log Out
       </button>
      </div>
     )}
    </div>
   ) : (
    <button className="auth-top-btn" onClick={()=>{setAuthOpen(true); setAuthError(''); setAuthTab('continue'); setAuthStep('email');}}>
     <User size={16}/>
     <span>Sign In</span>
    </button>
   )}
   <button className="cart-top" onClick={()=>setCartOpen(true)}><ShoppingBag size={19}/><span>Cart</span>{count>0&&<b>{count}</b>}</button>
  </header>
  <div className="main-layout"><main className="discovery">
   <section className="welcome"><div><span className="eyebrow">YOUR NEIGHBOURHOOD, SERVED</span><h1>Good food is <em>closer</em><br/>than you think.</h1><p>Local favourites, comforting classics, and new things to love.</p><div className="welcome-pills"><span><Clock3 size={15}/> 20–35 min average</span><span><Bike size={16}/> Live local discovery</span></div></div><div className="welcome-art"><img src="https://images.unsplash.com/photo-1547592180-85f173990554?auto=format&fit=crop&w=600&q=85" alt="Fresh colourful meal"/></div></section>
   {recommendations && Array.isArray(recommendations.items) && recommendations.items.length > 0 && !query && (
    <section className="recommendations-section">
     <div className="rec-head">
      <div className="rec-head-info">
       <span className="rec-eyebrow"><Sparkles size={13}/> {recommendations.source === 'personalized' ? 'RECOMMENDED FOR YOU' : 'COMMUNITY FAVOURITES'}</span>
       <h2>{recommendations.title || 'Previous Orders & Recommendations'}</h2>
       <p>{recommendations.subtitle || 'Dishes you love and frequently re-ordered favourites near you'}</p>
      </div>
      <div className="rec-source-pill">
       {recommendations.source === 'personalized' ? (
        <><RotateCcw size={13}/> Based on your order history</>
       ) : recommendations.source === 'previous_orders_trending' ? (
        <><Flame size={13}/> Neighbourhood Re-orders</>
       ) : (
        <><Star size={13}/> Chef's Top Rated</>
       )}
      </div>
     </div>
     <div className="rec-cards-container">
      {recommendations.items.map((item, idx) => {
       const badgeClass = item.badge === 'Past Favourite' ? 'past-fav' : item.badge === 'Most Re-ordered' ? 'reordered' : 'top-pick';
       const matchedRest = restaurants.find(r => String(r.id) === String(item.restaurantId));
       const itemIdStr = String(item.id);
       return (
        <article className="rec-card" key={item.id ?? idx}>
         <div className="rec-card-image">
          {item.badge && <span className={`rec-badge ${badgeClass}`}><Sparkles size={10}/> {item.badge}</span>}
          <img
           src={item.image || getFoodFallback(item.name, item.veg)}
           alt={item.name}
           loading="lazy"
           onError={e => {
            e.currentTarget.onerror = null;
            e.currentTarget.src = getFoodFallback(item.name, item.veg);
           }}
          />
          <span className={'veg-mark ' + (item.veg ? 'veg' : 'nonveg')}></span>
         </div>
         <div className="rec-card-body">
          {item.reason && (
           <div className="rec-reason">
            <Sparkles size={11}/>
            <span>{item.reason}</span>
           </div>
          )}
          <h4 className="rec-name" title={item.name}>{item.name}</h4>
          <button
           type="button"
           className="rec-restaurant"
           onClick={() => matchedRest && focusRestaurant(matchedRest)}
           title={`View ${item.restaurantName || matchedRest?.name || 'Restaurant'}`}
          >
           <span>★ {item.rating || matchedRest?.rating || '4.5'}</span> · {item.restaurantName || matchedRest?.name || 'Local Kitchen'}
          </button>
          <p className="rec-card-desc">{item.desc || 'Freshly prepared with quality ingredients'}</p>
          <div className="rec-footer">
           <span className="rec-price">{money(item.price)}</span>
           {cart[itemIdStr] ? (
            <div className="qty-control">
             <button onClick={() => change(itemIdStr, -1)}><Minus size={13}/></button>
             <b>{cart[itemIdStr]}</b>
             <button onClick={() => change(itemIdStr, 1)}><Plus size={13}/></button>
            </div>
           ) : (
            <button
             className="add-btn"
             onClick={() => {
              change(itemIdStr, 1);
              notify(`${item.name} added to cart`);
             }}
            >
             ADD <Plus size={13}/>
            </button>
           )}
          </div>
         </div>
        </article>
       );
      })}
     </div>
    </section>
   )}
   <section className="category-row">{categories.map(([c,emoji])=><button key={c} className={'category '+(category===c?'active':'')} onClick={()=>setCategory(c)}><span>{emoji}</span>{c}</button>)}</section>
   <div className="section-head"><div><span className="eyebrow">CURATED FOR YOU</span><h2>{query?`Results for “${query}”`:category==='All'?'Restaurants near you':`${category} around you`} <span className="count">{ordered.length}</span></h2></div><label className="sort"><SlidersHorizontal size={16}/><select value={sort} onChange={e=>setSort(e.target.value)}><option>Recommended</option><option>Rating</option><option>Fastest</option></select></label></div>
   <div className="restaurant-grid">{ordered.map(r=><article className="restaurant-card" key={r.id}><button className="restaurant-image" onClick={()=>focusRestaurant(r)}><img src={r.hero} alt={`${r.name} food`} loading="lazy"/><span className="offer">{r.offer}</span><span className="image-tag">{r.tag}</span></button><button className={'heart '+(liked.includes(r.id)?'liked':'')} onClick={()=>setLiked(p=>p.includes(r.id)?p.filter(x=>x!==r.id):[...p,r.id])} aria-label="Save restaurant"><Heart size={18} fill={liked.includes(r.id)?'currentColor':'none'}/></button><div className="restaurant-info" onClick={()=>focusRestaurant(r)}><div className="restaurant-title"><h3>{r.name}</h3><span className="rating"><Star size={13} fill="currentColor"/>{r.rating}</span></div><p>{r.cuisine}</p><div className="restaurant-meta"><span><Clock3 size={14}/>{r.time}</span><i>·</i><span>{r.fee}</span></div>{deliveryLocation&&Number.isFinite(Number(r.lat))&&Number.isFinite(Number(r.lng))&&<div className="restaurant-distance"><Navigation size={12}/>{haversineKm(deliveryLocation.lat,deliveryLocation.lng,Number(r.lat),Number(r.lng)).toFixed(1)} km </div>}</div><button className="view-menu" onClick={()=>setSelected(r)}>View menu <ArrowRight size={15}/></button></article>)}</div>
   {ordered.length===0&&<div className="empty-results"><span>🔎</span><h3>No matches just yet</h3><p>Try another dish or switch the category.</p><button className="primary" onClick={()=>{setQuery('');setCategory('All');}}>Show all restaurants</button></div>}
   <footer><span className="brand-mark tiny">f</span> FeastOS <span>•</span> Thoughtful food delivery, powered by smarter dispatch.</footer>
  </main>
  <aside className="map-panel"><div className="map-heading"><div><span className="eyebrow">EXPLORE YOUR AREA</span><h2>Nearby on the map</h2><p>{backendStatus==='connected'?'':backendStatus==='connecting'?'Connecting to FeastOS API…':'Demo data · C API offline'}</p></div><div className="map-tools"><button className="map-locate" onClick={refreshRestaurants} title="Refresh restaurant and menu data"><RefreshCw size={16}/></button><button className="map-locate" onClick={fitAllRestaurants} title="Fit all restaurant pins"><SlidersHorizontal size={16}/></button><button className="map-locate" onClick={locateMe} title="Find my location"><LocateFixed size={17}/></button></div></div>
   <form className="location-search" onSubmit={searchPlaces}><Search size={17}/><input ref={locationInputRef} value={locationQuery} onChange={e=>setLocationQuery(e.target.value)} placeholder="Search address, landmark or city…" aria-label="Search delivery location"/><button type="submit" disabled={locationSearchBusy} title="Search places">{locationSearchBusy?'…':<Search size={16}/>}</button><button type="button" className={pickDeliveryMode?'pick-active':''} onClick={()=>{setPickDeliveryMode(v=>!v);setLocationResults([]);notify(pickDeliveryMode?'Map picking cancelled':'Click anywhere on the map to set delivery location.');}}><MapPin size={14}/>{pickDeliveryMode?'Picking…':'Pick on map'}</button></form>
   {locationResults.length>0&&<div className="location-results">{locationResults.map(place=><button key={place.id} type="button" onClick={()=>applyDeliveryLocation(place)}><MapPin size={15}/><span>{place.label}</span></button>)}</div>}{deliveryLocation&&<div className="delivery-selection"><MapPin size={15}/><span><b>Deliver to:</b> {deliveryLocation.label}</span><button type="button" onClick={()=>{setDeliveryLocation(null);setRouteCoordinates([]);setRouteInfo(null);notify('Delivery location cleared.');}}>Clear</button></div>}
   <div className="map-wrap"><MapContainer center={center} zoom={13} zoomControl={false} scrollWheelZoom className={'leaflet-map '+(pickDeliveryMode?'map-picking':'')}><TileLayer attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors' url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"/><ZoomControl position="bottomright"/><Recenter center={center}/><FitRestaurants restaurants={restaurants} fitKey={fitKey}/><MapClickHandler enabled={pickDeliveryMode} onPick={applyDeliveryLocation}/>{restaurants.filter(r=>Number.isFinite(Number(r.lat))&&Number.isFinite(Number(r.lng))).map(r=><Marker key={r.id} position={[Number(r.lat),Number(r.lng)]} icon={pinIcon} eventHandlers={{click:()=>focusRestaurant(r)}}><Popup><div className="map-popup"><img src={r.hero} alt=""/><b>{r.name}</b><span>⭐ {r.rating} · {r.time}</span></div></Popup></Marker>)}{userLocation&&!deliveryLocation&&<CircleMarker center={userLocation} radius={8} pathOptions={{color:'#ffffff',weight:3,fillColor:'#2878e5',fillOpacity:1}}><Popup>Your current location</Popup></CircleMarker>}{deliveryLocation&&<Marker position={[deliveryLocation.lat,deliveryLocation.lng]} icon={deliveryPinIcon}><Popup><b>Delivery location</b><br/>{deliveryLocation.label}</Popup></Marker>}{routeCoordinates.length>1&&<Polyline positions={routeCoordinates} pathOptions={{color:'#e66b36',weight:5,opacity:0.85}}/>}</MapContainer><div className="map-hint"><Navigation size={14}/>{pickDeliveryMode?'Click the map to set your delivery location':'Drag to explore · scroll to zoom'}</div></div>
   <div className="map-selected"><span className="map-selected-icon"><MapPin size={18}/></span><div><b>{selected?selected.name:'Your food neighbourhood'}</b><span>{selected?`${selected.rating} ★ · ${selected.time}`:'Search a place or choose a point as your delivery location'}</span>{routeInfo&&selected&&routeInfo.restaurantId===String(selected.id)&&<span className="route-meta">Road route: {routeInfo.distance} km · about {routeInfo.duration} min</span>}</div>{selected&&<button className="route-button" onClick={showRoute} disabled={routeBusy||!deliveryLocation} title="Show road route from delivery location"><Navigation size={16}/>{routeBusy?'…':'Route'}</button>}</div>
   <div className="map-legend"><span><i className="legend-dot orange"></i> Restaurants</span><span><i className="legend-dot blue"></i> Delivery pin</span><button onClick={locateMe}><LocateFixed size={14}/></button></div>
  </aside></div>

  {selected&&<div className="modal-backdrop" onMouseDown={e=>e.target===e.currentTarget&&setSelected(null)}><section className="menu-modal"><div className="menu-cover"><img src={selected.hero} alt={`${selected.name} menu cover`}/><button className="close-modal" onClick={()=>setSelected(null)}><X size={19}/></button><span className="menu-offer">{selected.offer}</span></div><div className="menu-body"><span className="eyebrow">RESTAURANT MENU</span><div className="menu-title"><div><h2>{selected.name}</h2><p>{selected.cuisine}</p></div><span className="rating"><Star size={14} fill="currentColor"/>{selected.rating}</span></div><div className="menu-stats"><span><Clock3 size={15}/>{selected.time}</span><span><Bike size={16}/>{selected.fee}</span><span><Check size={15}/>Quality checked</span></div><div className="menu-section-title"><h3>Popular picks</h3><span>Freshly prepared for you</span></div><div className="menu-items">{(selected.items||[]).length===0?<p className="empty-menu">This restaurant has no available menu items yet.</p>:(selected.items||[]).map(item=><div className="menu-item" key={item.id}><img src={item.image||getFoodFallback(item.name,item.veg)} alt={item.name} loading="lazy" onError={e=>{e.currentTarget.onerror=null;e.currentTarget.src=getFoodFallback(item.name,item.veg);}}/><div className="menu-item-copy"><span className={'veg-mark '+(item.veg?'veg':'nonveg')}></span><h4>{item.name}</h4><b>{money(item.price)}</b><p>{item.desc}</p></div><div className="add-control">{cart[item.id]?<div className="qty-control"><button onClick={()=>change(item.id,-1)}><Minus size={14}/></button><b>{cart[item.id]}</b><button onClick={()=>change(item.id,1)}><Plus size={14}/></button></div>:<button className="add-btn" onClick={()=>{change(item.id,1);notify(`${item.name} added to cart`);}}>ADD <Plus size={14}/></button>}<small>Customisable</small></div></div>)}</div></div><div className="menu-footer"><span>{count} item{count===1?'':'s'} in your cart</span><button className="primary" onClick={()=>setCartOpen(true)}>View cart · {money(subtotal)} <ArrowRight size={17}/></button></div></section></div>}

  {cartOpen&&<div className="drawer-backdrop" onMouseDown={e=>e.target===e.currentTarget&&setCartOpen(false)}><aside className="cart-drawer"><div className="drawer-head"><div><span className="eyebrow">YOUR ORDER</span><h2>Your cart <span className="count">{count}</span></h2></div><button className="close-round" onClick={()=>setCartOpen(false)}><X size={20}/></button></div>{count===0?<div className="cart-empty"><div className="empty-bag"><ShoppingBag size={32}/></div><h3>Your cart is taking a little break</h3><p>Add something delicious from a nearby restaurant and it will show up here.</p><button className="primary" onClick={()=>setCartOpen(false)}>Explore restaurants <ArrowRight size={16}/></button></div>:<><div className="cart-items">{cartRows.map(i=><div className="cart-line" key={i.id}><img src={i.image||getFoodFallback(i.name,i.veg)} alt={i.name} onError={e=>{e.currentTarget.onerror=null;e.currentTarget.src=getFoodFallback(i.name,i.veg);}}/><div className="cart-line-info"><b>{i.name}</b><span>{i.restaurantName}</span><strong>{money(i.price)}</strong></div><div className="qty-control"><button onClick={()=>change(i.id,-1)}><Minus size={14}/></button><b>{cart[i.id]}</b><button onClick={()=>change(i.id,1)}><Plus size={14}/></button></div></div>)}</div><div className="cart-summary"><div><span>Item total</span><b>{money(subtotal)}</b></div><div><span>Delivery & handling</span><b>₹29</b></div><div><span>Platform fee</span><b>₹5</b></div><div className="total"><span>To pay</span><b>{money(subtotal+34)}</b></div>{deliveryLocation&&cartRows[0]&&<div className="delivery-order-distance"><MapPin size={14}/><span>Deliver to: {deliveryLocation.label}<br/>Approx. Distance: {haversineKm(deliveryLocation.lat,deliveryLocation.lng,Number(restaurants.find(r=>String(r.id)===String(cartRows[0].restaurantId))?.lat),Number(restaurants.find(r=>String(r.id)===String(cartRows[0].restaurantId))?.lng)).toFixed(1)} km</span></div>}<div className="demo-transaction-note"><strong>Choose payment method</strong><span>Razorpay confirms after verification. Cash on Delivery places your order immediately; pay cash when it arrives.</span></div><button className="primary checkout" onClick={placeRazorpayOrder} disabled={backendStatus!=='connected'||!deliveryLocation}>{backendStatus!=='connected'?'Connect database to pay':!deliveryLocation?'Choose delivery location first':'Pay with Razorpay'} <ArrowRight size={17}/></button><button className="secondary checkout" onClick={placeCodOrder} disabled={backendStatus!=='connected'||!deliveryLocation}>{backendStatus!=='connected'?'Connect database first':!deliveryLocation?'Choose delivery location first':'Cash on Delivery · Pay cash'} <ArrowRight size={17}/></button></div></>}</aside></div>}

  {ordersOpen&&<div className="drawer-backdrop" onMouseDown={e=>e.target===e.currentTarget&&setOrdersOpen(false)}><aside className="cart-drawer orders-drawer"><div className="drawer-head"><div><span className="eyebrow">YOUR ACCOUNT</span><h2>Past orders</h2></div><button className="close-round" onClick={()=>setOrdersOpen(false)} aria-label="Close past orders"><X size={20}/></button></div>{ordersLoading?<p className="orders-message">Loading orders from PostgreSQL…</p>:ordersError?<div className="orders-message"><h3>Order history unavailable</h3><p>{ordersError}</p><p>The backend must provide <code>GET /api/orders/history</code>.</p><button className="primary" onClick={loadPastOrders}><RefreshCw size={16}/> Try again</button></div>:pastOrders.length===0?<div className="cart-empty"><div className="empty-bag"><ClipboardList size={32}/></div><h3>No past orders found</h3><p>Orders saved in the database will appear here.</p></div>:<div className="past-orders-list">{pastOrders.map((order,index)=><article className="past-order-card" key={order.order_id??order.id??index}><div className="past-order-heading"><strong>Order #{order.order_id??order.id}</strong><span>{order.status||'Placed'}</span></div><p>{order.restaurant_name||'Restaurant details unavailable'}</p>{order.created_at&&<small>{formatDateTime(order.created_at)}</small>}{order.user_email&&<span className="order-user-tag"><Mail size={11}/> {order.user_email}</span>}{Array.isArray(order.items)&&order.items.length>0&&<ul>{order.items.map((item,itemIndex)=><li key={item.item_id??item.id??itemIndex}>{item.quantity||1} × {item.item_name||item.item_name_snapshot||item.name||'Menu item'}</li>)}</ul>}{Number.isFinite(Number(order.total_paise))&&<strong className="past-order-total">Total: {money(Number(order.total_paise)/100)}</strong>}{order.payment_method&&<small>Method: {String(order.payment_method).toUpperCase()==='COD'?'Cash on Delivery':order.payment_method}</small>}{order.payment_status&&<small>Payment: {order.payment_status}</small>}{(order.assignment_status||order.agent_name||order.estimated_completion_at||order.distance_km!=null)&&<div className="delivery-assignment-card"><div className="delivery-assignment-title"><Bike size={16}/><strong>Delivery dispatch</strong><span className={'delivery-status-pill '+String(order.assignment_status||'waiting').toLowerCase()}>{String(order.assignment_status||'Awaiting assignment').replaceAll('_',' ')}</span></div>{order.agent_name&&<p><b>Delivery agent:</b> {order.agent_name}{order.agent_status?` · ${String(order.agent_status).replaceAll('_',' ')}`:''}</p>}{order.distance_km!=null&&Number.isFinite(Number(order.distance_km))&&<p><b>Estimated distance:</b> {Number(order.distance_km).toFixed(1)} km</p>}{order.estimated_completion_at&&<p><b>Estimated completion:</b> {formatDateTime(order.estimated_completion_at)}</p>}{order.assignment_status==='delivered'&&order.completed_at&&<p><b>Delivered:</b> {formatDateTime(order.completed_at)} · agent released</p>}{!order.assignment_status&&!order.agent_name&&<p>Waiting for an available delivery agent.</p>}<small></small></div>}{Array.isArray(order.items)&&order.items.length>0&&<button type="button" className="reorder-btn" onClick={()=>handleReorder(order)}><RotateCcw size={13}/> Reorder items</button>}</article>)}</div>}
   <details className="scheduling-panel"><summary className="scheduling-title"><BarChart3 size={19}/><h3>OS scheduling comparison</h3><span className="scheduling-toggle-hint"></span></summary><div className="scheduling-panel-content"><p className="scheduling-description">Compare FCFS and priority scheduling using saved orders.</p>{schedulingLoading?<p>Calculating scheduling statistics…</p>:schedulingError?<div className="scheduling-unavailable"><p>{schedulingError}</p><p>The backend must provide <code>GET /api/orders/scheduling-stats</code>.</p></div>:schedulingStats?<div className="scheduling-results">
{(schedulingStats.algorithms||schedulingStats.results||[]).length===0
 ? <div className="scheduling-empty-state">
     <h4 style={{ margin: "0 0 8px", paddingLeft: "2px" }}>
  No eligible orders to compare
</h4>
     <p>Orders already assigned to a delivery agent are excluded from FCFS and priority scheduling comparisons.</p>
     <p>New unassigned orders will appear here when available.</p>
   </div>
 : (schedulingStats.algorithms||schedulingStats.results||[]).map((algorithm,index)=><article className="scheduling-result-card" key={algorithm.algorithm_name||algorithm.name||index}><h4>{algorithm.algorithm_name||algorithm.name||`Algorithm ${index+1}`}</h4><div><span>Orders</span><strong>{algorithm.totalOrders??algorithm.total_orders??algorithm.total_orders_count??'—'}</strong></div><div><span>Average waiting time</span><strong>{(algorithm.averageWaitingTime??algorithm.avg_waiting_time)==null?'—':`${Number(algorithm.averageWaitingTime??algorithm.avg_waiting_time).toFixed(2)} min`}</strong></div><div><span>Average turnaround time</span><strong>{(algorithm.averageTurnaroundTime??algorithm.avg_turnaround_time)==null?'—':`${Number(algorithm.averageTurnaroundTime??algorithm.avg_turnaround_time).toFixed(2)} min`}</strong></div><div><span>Average response time</span><strong>{(algorithm.averageResponseTime??algorithm.avg_response_time)==null?'—':`${Number(algorithm.averageResponseTime??algorithm.avg_response_time).toFixed(2)} min`}</strong></div><div><span>Resource utilization</span><strong>{(algorithm.resourceUtilization??algorithm.resource_utilization)==null?'—':`${Number(algorithm.resourceUtilization??algorithm.resource_utilization).toFixed(1)}%`}</strong></div></article>)}
{Array.isArray(schedulingStats.priorityBreakdown)&&schedulingStats.priorityBreakdown.length>0&&
 <details className="scheduling-timeline priority-breakdown" open>
  
  <div className="priority-breakdown-list">
   {schedulingStats.priorityBreakdown.map(order=><article className="scheduling-result-card" key={order.orderId}>
    <h4>Order #{order.orderId} · Priority {order.priorityScore}/100</h4>
    <div><span>Raw perishability score</span><strong>{order.perishabilityScore}</strong></div>
    <div><span>Preparation time</span><strong>{order.preparationTimeMin} min</strong></div>
    <div><span>Preparation points</span><strong>{order.preparationPoints}</strong></div>
    <div><span>Delivery distance</span><strong>{Number(order.distanceKm).toFixed(1)} km</strong></div>
    <div><span>Distance points</span><strong>{order.distancePoints}</strong></div>
    <div><span>Final calculation</span><strong>{order.perishabilityScore} + {order.preparationPoints} + {order.distancePoints} = {order.priorityScore}</strong></div>
   </article>)}
  </div>
 </details>}
{(schedulingStats.algorithms||schedulingStats.results||[]).some(algorithm=>Array.isArray(algorithm.timeline)&&algorithm.timeline.length>0)&&<details className="scheduling-timeline"><pre>{JSON.stringify((schedulingStats.algorithms||schedulingStats.results||[]).map(algorithm=>({name:algorithm.algorithm_name||algorithm.name||'Scheduling algorithm',timeline:algorithm.timeline||[]})),null,2)}</pre></details>}</div>:<p>No scheduling results loaded yet. Refresh to request the latest results.</p>}<button className="secondary-button" onClick={loadSchedulingStats}><RefreshCw size={15}/> Refresh scheduling statistics</button></div></details>
  </aside></div>}

  {authOpen && (
   <div className="modal-backdrop" onMouseDown={e=>e.target===e.currentTarget&&setAuthOpen(false)}>
    <div className="auth-modal">
     <div className="auth-modal-header">
      <div className="auth-brand">
       <span className="brand-mark small">f</span>
       <div>
        <h3>Welcome to FeastOS</h3>
       </div>
      </div>
      <button className="close-round" onClick={()=>setAuthOpen(false)} aria-label="Close dialog">
       <X size={18}/>
      </button>
     </div>

     <div className="auth-tabs">
      <button
       type="button"
       className={'auth-tab ' + (authTab === 'continue' ? 'active' : '')}
       onClick={() => { setAuthTab('continue'); setAuthStep('email'); setAuthError(''); }}
      >
       Continue with Email
      </button>
      <button
       type="button"
       className={'auth-tab ' + (authTab === 'login' ? 'active' : '')}
       onClick={() => { setAuthTab('login'); setAuthError(''); }}
      >
       Sign In
      </button>
      <button
       type="button"
       className={'auth-tab ' + (authTab === 'signup' ? 'active' : '')}
       onClick={() => { setAuthTab('signup'); setAuthError(''); }}
      >
       Create Account
      </button>
     </div>

     <form onSubmit={handleAuthSubmit} className="auth-form">
      {authError && <div className="auth-error-banner">{authError}</div>}

      <div className="auth-field">
       <label>Email address</label>
       <div className="auth-input-box">
        <Mail size={16}/>
        <input
         type="email"
         autoFocus
         required
         placeholder="you@example.com"
         value={authEmail}
         onChange={e => { setAuthEmail(e.target.value); setAuthError(''); }}
        />
       </div>
      </div>

      {(authTab === 'signup' || (authTab === 'continue' && authStep === 'signup')) && (
       <div className="auth-field">
        <label>Your full name</label>
        <div className="auth-input-box">
         <User size={16}/>
         <input
          type="text"
          placeholder="e.g. Ankit Sharma"
          value={authName}
          onChange={e => setAuthName(e.target.value)}
         />
        </div>
       </div>
      )}

      {(authTab !== 'continue' || authStep !== 'email') && (
       <div className="auth-field">
        <label>
         {authTab === 'signup' || authStep === 'signup' ? 'Create a password' : 'Enter your password'}
        </label>
        <div className="auth-input-box">
         <Lock size={16}/>
         <input
          type={showPassword ? 'text' : 'password'}
          required
          minLength={4}
          placeholder="At least 4 characters"
          value={authPassword}
          onChange={e => setAuthPassword(e.target.value)}
         />
         <button
          type="button"
          className="pwd-toggle"
          onClick={() => setShowPassword(v => !v)}
          title={showPassword ? 'Hide password' : 'Show password'}
         >
          {showPassword ? <EyeOff size={15}/> : <Eye size={15}/>}
         </button>
        </div>
       </div>
      )}

      <button type="submit" className="primary auth-submit-btn" disabled={authLoading}>
       {authLoading ? 'Please wait…' : (
        authTab === 'continue' && authStep === 'email'
         ? 'Continue with Email'
         : authTab === 'signup' || authStep === 'signup'
           ? 'Create Account'
           : 'Log In'
       )}
       <ArrowRight size={16}/>
      </button>

      {authTab === 'continue' && authStep !== 'email' && (
       <button
        type="button"
        className="auth-back-link"
        onClick={() => { setAuthStep('email'); setAuthPassword(''); setAuthError(''); }}
       >
        ← Use a different email
       </button>
      )}
     </form>

     <div className="auth-modal-footer">
     </div>
    </div>
   </div>
  )}

  {toast&&<div className="toast"><Check size={17}/>{toast}</div>}
 </div>;
}

createRoot(document.getElementById('root')).render(<App/>);
