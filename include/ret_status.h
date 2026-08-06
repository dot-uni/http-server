#ifndef RET_STATUS_INCLUDED
#define RET_STATUS_INCLUDED

#include <cstdint>
#include <string>
#include <string_view>
#include "status.h"


namespace http {

// Application-level result code returned in the `retCode` field of every response.
// Underlying type is uint32_t since all codes are small non-negative integers.
enum class retCode : unsigned {
    Success                        = 0,     // HTTP 200/201/202
    InvalidJsonOrParams            = 10001, // HTTP 400
    AuthError                      = 10002, // HTTP 401
    Forbidden                      = 10003, // HTTP 403 (no scope/permission)
    NotFound                       = 10005, // HTTP 404
    ConflictDuplicateClientOrderId = 10006, // HTTP 409
    InvalidSimulationState         = 10007, // HTTP 422
    RateLimitExceeded              = 10008, // HTTP 429
    InternalError                  = 10009, // HTTP 500
    InsufficientBalance            = 10010, // HTTP 400
    PriceOrQtyNotAligned           = 10011, // HTTP 400 (not multiple of tickSize/qtyStep)
    PositionOrOrderLimitExceeded   = 10012, // HTTP 422
    UnknownSymbolOrDataset         = 10013, // HTTP 404
    SimulationNotRunning           = 10014, // HTTP 422
    ClientOrderIdAlreadyUsed       = 10015, // HTTP 409
    SessionOrSignatureExpired      = 10016, // HTTP 401
    InvalidSignature               = 10017, // HTTP 401
    ClockSkewExceedsRecvWindow     = 10018, // HTTP 401
    PostOnlyWouldCross             = 10019, // HTTP 422
};

// Returns the HTTP status code most commonly associated with a retCode.
// Note: Success (0) is context-dependent — 200 for GET/query, 201 for creation,
// 202 for an accepted async command. Pass the actual HTTP status separately
// when handling Success; the value below is just a sensible default.
constexpr http::status toHttpStatus(retCode code) {
    switch (static_cast<retCode>(code)) {
        case retCode::Success:                         return http::status::ok;
        case retCode::InvalidJsonOrParams:             return http::status::bad_request;
        case retCode::AuthError:                       return http::status::unauthorized;
        case retCode::Forbidden:                       return http::status::forbidden;
        case retCode::NotFound:                        return http::status::not_found;
        case retCode::ConflictDuplicateClientOrderId:  return http::status::conflict;
        case retCode::InvalidSimulationState:          return http::status::unprocessable_entity;
        case retCode::RateLimitExceeded:               return http::status::too_many_requests;
        case retCode::InternalError:                   return http::status::internal_server_error;
        case retCode::InsufficientBalance:             return http::status::bad_request;
        case retCode::PriceOrQtyNotAligned:            return http::status::bad_request;
        case retCode::PositionOrOrderLimitExceeded:    return http::status::unprocessable_entity;
        case retCode::UnknownSymbolOrDataset:          return http::status::not_found;
        case retCode::SimulationNotRunning:            return http::status::unprocessable_entity;
        case retCode::ClientOrderIdAlreadyUsed:        return http::status::conflict;
        case retCode::SessionOrSignatureExpired:       return http::status::unauthorized;
        case retCode::InvalidSignature:                return http::status::unauthorized;
        case retCode::ClockSkewExceedsRecvWindow:      return http::status::unauthorized;
        case retCode::PostOnlyWouldCross:              return http::status::unprocessable_entity;
        default:
            break;
    }
    return http::status::internal_server_error;
}

// Human-readable description 
constexpr std::string_view retMesg(retCode code) {
    switch (static_cast<retCode>(code)) {
        case retCode::Success:                         return "Success";
        case retCode::InvalidJsonOrParams:             return "Invalid JSON or parameters";
        case retCode::AuthError:                       return "Authorization error";
        case retCode::Forbidden:                       return "Forbidden (insufficient scope)";
        case retCode::NotFound:                        return "Resource not found";
        case retCode::ConflictDuplicateClientOrderId:  return "Conflict or duplicated clientOrderId";
        case retCode::InvalidSimulationState:          return "Command not valid for current simulation state";
        case retCode::RateLimitExceeded:               return "Rate limit exceeded";
        case retCode::InternalError:                   return "Internal error";
        case retCode::InsufficientBalance:             return "Insufficient balance";
        case retCode::PriceOrQtyNotAligned:            return "Price/quantity not aligned with tickSize/qtyStep";
        case retCode::PositionOrOrderLimitExceeded:    return "Position or order size limit exceeded";
        case retCode::UnknownSymbolOrDataset:          return "Unknown symbol or dataset";
        case retCode::SimulationNotRunning:            return "Simulation is not in Running state";
        case retCode::ClientOrderIdAlreadyUsed:        return "clientOrderId already used in this simulation";
        case retCode::SessionOrSignatureExpired:       return "Session/signature expired";
        case retCode::InvalidSignature:                return "Invalid signature";
        case retCode::ClockSkewExceedsRecvWindow:      return "Clock skew exceeds recvWindow";
        case retCode::PostOnlyWouldCross:              return "PostOnly order would cross the book (WouldCross)";
        default:
            break;
    }
    return "<unknown-retCode>";
}

} // namespace http

#endif