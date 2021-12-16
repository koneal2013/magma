/**
 * Copyright 2020 The Magma Authors.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "lte/gateway/c/core/oai/tasks/amf/amf_app_state_converter.h"
#include <vector>
#include <memory>
extern "C" {
#include "lte/gateway/c/core/oai/lib/message_utils/bytes_to_ie.h"
#include "lte/gateway/c/core/oai/common/conversions.h"
#include "lte/gateway/c/core/oai/common/dynamic_memory_check.h"
#include "lte/gateway/c/core/oai/lib/message_utils/ie_to_bytes.h"
#include "lte/gateway/c/core/oai/common/log.h"
}

using magma::lte::oai::EmmContext;
using magma::lte::oai::MmeNasState;
namespace magma5g {

AmfNasStateConverter::AmfNasStateConverter()  = default;
AmfNasStateConverter::~AmfNasStateConverter() = default;

// HelperFunction: Converts guti_m5_t to std::string
std::string AmfNasStateConverter::amf_app_convert_guti_m5_to_string(
    const guti_m5_t& guti) {
#define GUTI_STRING_LEN 25
  char* temp_str =
      reinterpret_cast<char*>(calloc(1, sizeof(char) * GUTI_STRING_LEN));
  snprintf(
      temp_str, GUTI_STRING_LEN, "%x%x%x%x%x%x%02x%04x%04x%08x",
      guti.guamfi.plmn.mcc_digit1, guti.guamfi.plmn.mcc_digit2,
      guti.guamfi.plmn.mcc_digit3, guti.guamfi.plmn.mnc_digit1,
      guti.guamfi.plmn.mnc_digit2, guti.guamfi.plmn.mnc_digit3,
      guti.guamfi.amf_regionid, guti.guamfi.amf_set_id, guti.guamfi.amf_pointer,
      guti.m_tmsi);
  std::string guti_str(temp_str);
  free(temp_str);
  return guti_str;
}

// HelperFunction: Converts std:: string back to guti_m5_t
void AmfNasStateConverter::amf_app_convert_string_to_guti_m5(
    const std::string& guti_str, guti_m5_t* guti_m5_p) {
  int idx                   = 0;
  std::size_t chars_to_read = 1;
#define HEX_BASE_VAL 16
  guti_m5_p->guamfi.plmn.mcc_digit1 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  guti_m5_p->guamfi.plmn.mcc_digit2 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  guti_m5_p->guamfi.plmn.mcc_digit3 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  guti_m5_p->guamfi.plmn.mnc_digit1 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  guti_m5_p->guamfi.plmn.mnc_digit2 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  guti_m5_p->guamfi.plmn.mnc_digit3 = std::stoul(
      guti_str.substr(idx++, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  chars_to_read                  = 2;
  guti_m5_p->guamfi.amf_regionid = std::stoul(
      guti_str.substr(idx, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  idx += chars_to_read;
  chars_to_read                = 4;
  guti_m5_p->guamfi.amf_set_id = std::stoul(
      guti_str.substr(idx, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  idx += chars_to_read;
  chars_to_read                 = 4;
  guti_m5_p->guamfi.amf_pointer = std::stoul(
      guti_str.substr(idx, chars_to_read), &chars_to_read, HEX_BASE_VAL);
  idx += chars_to_read;
  chars_to_read     = 8;
  guti_m5_p->m_tmsi = std::stoul(
      guti_str.substr(idx, chars_to_read), &chars_to_read, HEX_BASE_VAL);
}
// Converts Map<guti_m5_t,uint64_t> to proto
void AmfNasStateConverter::map_guti_uint64_to_proto(
    const map_guti_m5_uint64_t guti_map,
    google::protobuf::Map<std::string, uint64_t>* proto_map) {
  std::string guti_str;
  for (const auto& elm : guti_map.umap) {
    guti_str               = amf_app_convert_guti_m5_to_string(elm.first);
    (*proto_map)[guti_str] = elm.second;
  }
}

// Converts Proto to Map<guti_m5_t,uint64_t>
void AmfNasStateConverter::proto_to_guti_map(
    const google::protobuf::Map<std::string, uint64_t>& proto_map,
    map_guti_m5_uint64_t* guti_map) {
  for (auto const& kv : proto_map) {
    amf_ue_ngap_id_t amf_ue_ngap_id = kv.second;
    std::unique_ptr<guti_m5_t> guti = std::make_unique<guti_m5_t>();
    memset(guti.get(), 0, sizeof(guti_m5_t));
    // Converts guti to string.
    amf_app_convert_string_to_guti_m5(kv.first, guti.get());

    guti_m5_t guti_received = *guti.get();
    magma::map_rc_t m_rc    = guti_map->insert(guti_received, amf_ue_ngap_id);
    if (m_rc != magma::MAP_OK) {
      OAILOG_ERROR(
          LOG_AMF_APP,
          "Failed to insert amf_ue_ngap_id %lu in GUTI table, error: %s\n",
          amf_ue_ngap_id, map_rc_code2string(m_rc).c_str());
    }
  }
}

/*********************************************************
 *                AMF app state<-> Proto                  *
 * Functions to serialize/desearialize AMF app state      *
 * The caller is responsible for all memory management    *
 **********************************************************/

void AmfNasStateConverter::state_to_proto(
    const amf_app_desc_t* amf_nas_state_p, MmeNasState* state_proto) {
  OAILOG_FUNC_IN(LOG_AMF_APP);
  state_proto->set_mme_app_ue_s1ap_id_generator(
      amf_nas_state_p->amf_app_ue_ngap_id_generator);

  // These Functions are to be removed as part of the stateless enhancement
  // maps to proto
  auto amf_ue_ctxts_proto = state_proto->mutable_mme_ue_contexts();
  OAILOG_DEBUG(LOG_AMF_APP, "IMSI table to proto");
  magma::lte::StateConverter::map_uint64_uint64_to_proto(
      amf_nas_state_p->amf_ue_contexts.imsi_amf_ue_id_htbl,
      amf_ue_ctxts_proto->mutable_imsi_ue_id_htbl());
  magma::lte::StateConverter::map_uint64_uint64_to_proto(
      amf_nas_state_p->amf_ue_contexts.tun11_ue_context_htbl,
      amf_ue_ctxts_proto->mutable_tun11_ue_id_htbl());
  magma::lte::StateConverter::map_uint64_uint64_to_proto(
      amf_nas_state_p->amf_ue_contexts.gnb_ue_ngap_id_ue_context_htbl,
      amf_ue_ctxts_proto->mutable_enb_ue_id_ue_id_htbl());
  map_guti_uint64_to_proto(
      amf_nas_state_p->amf_ue_contexts.guti_ue_context_htbl,
      amf_ue_ctxts_proto->mutable_guti_ue_id_htbl());
  OAILOG_FUNC_OUT(LOG_AMF_APP);
}

void AmfNasStateConverter::proto_to_state(
    const MmeNasState& state_proto, amf_app_desc_t* amf_nas_state_p) {
  OAILOG_FUNC_IN(LOG_AMF_APP);
  amf_nas_state_p->amf_app_ue_ngap_id_generator =
      state_proto.mme_app_ue_s1ap_id_generator();

  if (amf_nas_state_p->amf_app_ue_ngap_id_generator == 0) {  // uninitialized
    amf_nas_state_p->amf_app_ue_ngap_id_generator = 1;
  }
  OAILOG_INFO(LOG_AMF_APP, "Done reading AMF statistics from data store");

  magma::lte::oai::MmeUeContext amf_ue_ctxts_proto =
      state_proto.mme_ue_contexts();

  amf_ue_context_t* amf_ue_ctxt_state = &amf_nas_state_p->amf_ue_contexts;

  // proto to maps
  OAILOG_INFO(LOG_AMF_APP, "Hashtable AMF UE ID => IMSI");
  proto_to_map_uint64_uint64(
      amf_ue_ctxts_proto.imsi_ue_id_htbl(),
      &amf_ue_ctxt_state->imsi_amf_ue_id_htbl);
  proto_to_map_uint64_uint64(
      amf_ue_ctxts_proto.tun11_ue_id_htbl(),
      &amf_ue_ctxt_state->tun11_ue_context_htbl);
  proto_to_map_uint64_uint64(
      amf_ue_ctxts_proto.enb_ue_id_ue_id_htbl(),
      &amf_ue_ctxt_state->gnb_ue_ngap_id_ue_context_htbl);

  proto_to_guti_map(
      amf_ue_ctxts_proto.guti_ue_id_htbl(),
      &amf_ue_ctxt_state->guti_ue_context_htbl);
  OAILOG_FUNC_OUT(LOG_AMF_APP);
}

void AmfNasStateConverter::ue_to_proto(
    const ue_m5gmm_context_t* ue_ctxt,
    magma::lte::oai::UeContext* ue_ctxt_proto) {
  ue_m5gmm_context_to_proto(ue_ctxt, ue_ctxt_proto);
}

void AmfNasStateConverter::proto_to_ue(
    const magma::lte::oai::UeContext& ue_ctxt_proto,
    ue_m5gmm_context_t* ue_ctxt) {
  proto_to_ue_m5gmm_context(ue_ctxt_proto, ue_ctxt);
}

/*********************************************************
 *                UE Context <-> Proto                    *
 * Functions to serialize/desearialize UE context         *
 * The caller needs to acquire a lock on UE context       *
 **********************************************************/

void AmfNasStateConverter::ue_m5gmm_context_to_proto(
    const ue_m5gmm_context_t* state_ue_m5gmm_context,
    magma::lte::oai::UeContext* ue_context_proto) {
  // Actual implementation logic will be added as part of upcoming pr
}

void AmfNasStateConverter::proto_to_ue_m5gmm_context(
    const magma::lte::oai::UeContext& ue_context_proto,
    ue_m5gmm_context_t* state_ue_m5gmm_context) {
  // Actual implementation logic will be added as part of upcoming pr
}

void AmfNasStateConverter::tai_to_proto(
    const tai_t* state_tai, magma::lte::oai::Tai* tai_proto) {
  OAILOG_DEBUG(
      LOG_MME_APP, "State PLMN " PLMN_FMT "to proto",
      PLMN_ARG(&state_tai->plmn));
  char plmn_array[PLMN_BYTES];
  plmn_array[0] = static_cast<char>(state_tai->plmn.mcc_digit1 + ASCII_ZERO);
  plmn_array[1] = static_cast<char>(state_tai->plmn.mcc_digit2 + ASCII_ZERO);
  plmn_array[2] = static_cast<char>(state_tai->plmn.mcc_digit3 + ASCII_ZERO);
  plmn_array[3] = static_cast<char>(state_tai->plmn.mnc_digit1 + ASCII_ZERO);
  plmn_array[4] = static_cast<char>(state_tai->plmn.mnc_digit2 + ASCII_ZERO);
  plmn_array[5] = static_cast<char>(state_tai->plmn.mnc_digit3 + ASCII_ZERO);
  tai_proto->set_mcc_mnc(plmn_array);
  tai_proto->set_tac(state_tai->tac);
}

void AmfNasStateConverter::proto_to_tai(
    const magma::lte::oai::Tai& tai_proto, tai_t* state_tai) {
  state_tai->plmn.mcc_digit1 =
      static_cast<int>(tai_proto.mcc_mnc()[0]) - ASCII_ZERO;
  state_tai->plmn.mcc_digit2 =
      static_cast<int>(tai_proto.mcc_mnc()[1]) - ASCII_ZERO;
  state_tai->plmn.mcc_digit3 =
      static_cast<int>(tai_proto.mcc_mnc()[2]) - ASCII_ZERO;
  state_tai->plmn.mnc_digit1 =
      static_cast<int>(tai_proto.mcc_mnc()[3]) - ASCII_ZERO;
  state_tai->plmn.mnc_digit2 =
      static_cast<int>(tai_proto.mcc_mnc()[4]) - ASCII_ZERO;
  state_tai->plmn.mnc_digit3 =
      static_cast<int>(tai_proto.mcc_mnc()[5]) - ASCII_ZERO;
  state_tai->tac = tai_proto.tac();
  OAILOG_DEBUG(
      LOG_MME_APP, "State PLMN " PLMN_FMT "from proto",
      PLMN_ARG(&state_tai->plmn));
}

void AmfNasStateConverter::amf_context_to_proto(
    const amf_context_t* amf_ctx, EmmContext* emm_context_proto) {
  emm_context_proto->set_imsi64(amf_ctx->imsi64);
  identity_tuple_to_proto<imsi_t>(
      &amf_ctx->imsi, emm_context_proto->mutable_imsi(), IMSI_BCD8_SIZE);
  emm_context_proto->set_saved_imsi64(amf_ctx->saved_imsi64);
  identity_tuple_to_proto<imei_t>(
      &amf_ctx->imei, emm_context_proto->mutable_imei(), IMEI_BCD8_SIZE);
  identity_tuple_to_proto<imeisv_t>(
      &amf_ctx->imeisv, emm_context_proto->mutable_imeisv(), IMEISV_BCD8_SIZE);
  emm_context_proto->set_emm_cause(amf_ctx->amf_cause);
  emm_context_proto->set_emm_fsm_state(amf_ctx->amf_fsm_state);
  emm_context_proto->set_attach_type(amf_ctx->m5gsregistrationtype);

  emm_context_proto->set_member_present_mask(amf_ctx->member_present_mask);
  emm_context_proto->set_member_valid_mask(amf_ctx->member_valid_mask);
  emm_context_proto->set_is_dynamic(amf_ctx->is_dynamic);
  emm_context_proto->set_is_attached(amf_ctx->is_registered);
  emm_context_proto->set_is_initial_identity_imsi(
      amf_ctx->is_initial_identity_imsi);
  emm_context_proto->set_is_guti_based_attach(
      amf_ctx->is_guti_based_registered);
  emm_context_proto->set_is_imsi_only_detach(amf_ctx->is_imsi_only_detach);
  tai_to_proto(
      &amf_ctx->originating_tai, emm_context_proto->mutable_originating_tai());
  emm_context_proto->set_ksi(amf_ctx->ksi);
}

void AmfNasStateConverter::proto_to_amf_context(
    const EmmContext& emm_context_proto, amf_context_t* amf_ctx) {
  amf_ctx->imsi64 = emm_context_proto.imsi64();
  proto_to_identity_tuple<imsi_t>(
      emm_context_proto.imsi(), &amf_ctx->imsi, IMSI_BCD8_SIZE);
  amf_ctx->saved_imsi64 = emm_context_proto.saved_imsi64();

  proto_to_identity_tuple<imei_t>(
      emm_context_proto.imei(), &amf_ctx->imei, IMEI_BCD8_SIZE);
  proto_to_identity_tuple<imeisv_t>(
      emm_context_proto.imeisv(), &amf_ctx->imeisv, IMEISV_BCD8_SIZE);

  amf_ctx->amf_cause     = emm_context_proto.emm_cause();
  amf_ctx->amf_fsm_state = (amf_fsm_state_t) emm_context_proto.emm_fsm_state();
  amf_ctx->m5gsregistrationtype = emm_context_proto.attach_type();
  amf_ctx->member_present_mask  = emm_context_proto.member_present_mask();
  amf_ctx->member_valid_mask    = emm_context_proto.member_valid_mask();
  amf_ctx->is_dynamic           = emm_context_proto.is_dynamic();
  amf_ctx->is_registered        = emm_context_proto.is_attached();
  amf_ctx->is_initial_identity_imsi =
      emm_context_proto.is_initial_identity_imsi();
  amf_ctx->is_guti_based_registered = emm_context_proto.is_guti_based_attach();
  amf_ctx->is_imsi_only_detach      = emm_context_proto.is_imsi_only_detach();
  proto_to_tai(emm_context_proto.originating_tai(), &amf_ctx->originating_tai);
  amf_ctx->ksi = emm_context_proto.ksi();
}
}  // namespace magma5g
