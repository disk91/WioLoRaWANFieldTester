export interface ApplicationIds {
  application_id: string
}

export interface EndDeviceIds {
  device_id: string
  application_ids: ApplicationIds
  dev_eui: string
  join_eui: string
  dev_addr: string
}

export interface GatewayIds {
  gateway_id: string
  eui: string
}

export interface Location {
  latitude: number
  longitude: number
  altitude: number
  source: string
}

export interface RxMetadata {
  gateway_ids: GatewayIds
  time: Date
  timestamp: number
  rssi: number
  channel_rssi: number
  snr: number
  location: Location
  uplink_token: string
  channel_index: number
}

export interface Lora {
  bandwidth: number
  spreading_factor: number
}

export interface DataRate {
  lora: Lora
}

export interface Settings {
  data_rate: DataRate
  coding_rate: string
  frequency: string
  timestamp: number
  time: Date
}

export interface FrmPayload {
  latitude: number
  longitude: number
  altitude: number
  source: string
}

export interface Locations {
  'frm-payload': FrmPayload
}

export interface NetworkIds {
  net_id: string
  tenant_id: string
  cluster_id: string
  cluster_address: string
}

export interface UplinkMessage {
  session_key_id: string
  f_port: number
  f_cnt: number
  frm_payload: string
  decoded_payload: any
  rx_metadata: RxMetadata[]
  settings: Settings
  received_at: string
  consumed_airtime: string
  locations: Locations
  network_ids: NetworkIds
}

export interface TTNMessage {
  end_device_ids: EndDeviceIds
  correlation_ids: string[]
  received_at: string
  uplink_message: UplinkMessage
}
