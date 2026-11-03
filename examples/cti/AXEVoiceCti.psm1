# Local CTI client for Windows PowerShell 5.1 or newer.
# Authentication secrets are read only from process environment.
Set-StrictMode -Version Latest
$script:BaseUrl = 'http://127.0.0.1:27652'

function Get-AXVCtiHeaders {
    $token=[Environment]::GetEnvironmentVariable('AXV_LOCAL_API_TOKEN')
    if ([string]::IsNullOrWhiteSpace($token)) {
        throw 'AXV_LOCAL_API_TOKEN must be configured'
    }
    if ($token.Length -lt 32 -or $token.Length -gt 128) {
        throw 'AXV_LOCAL_API_TOKEN has invalid length'
    }
    return @{ Authorization = "Bearer $token" }
}

function Get-AXVCtiCapabilities {
    Invoke-RestMethod -Method Get -Uri "$script:BaseUrl/v1/capabilities" -Headers (Get-AXVCtiHeaders)
}

function Start-AXVCtiCall {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][ValidateNotNullOrEmpty()][string]$Destination,
        [string]$IdempotencyKey = ([guid]::NewGuid().ToString('N'))
    )
    $cap=Get-AXVCtiCapabilities
    if (!$cap.features.click_to_call -or !$cap.features.idempotency_key) {
        throw 'AXE Voice does not support idempotent click-to-call'
    }
    if ($IdempotencyKey -cnotmatch '^[A-Za-z0-9_-]{16,64}$') {
        throw 'Idempotency key must be 16 to 64 ASCII letters, digits, dash or underscore'
    }
    $headers=Get-AXVCtiHeaders
    $headers['Idempotency-Key']=$IdempotencyKey
    $body=ConvertTo-Json -InputObject @{ destination = $Destination } -Compress
    # Reuse the same key after a network timeout to avoid accidental redial.
    # History is bounded and is not preserved across application restarts.
    Invoke-RestMethod -Method Post -Uri "$script:BaseUrl/v1/calls" -Headers $headers -ContentType 'application/json' -Body $body
}

function Get-AXVCtiRequest {
    [CmdletBinding()]
    param([Parameter(Mandatory)][ValidateRange(1,[long]::MaxValue)][long]$RequestId)
    Invoke-RestMethod -Method Get -Uri "$script:BaseUrl/v1/requests/$RequestId" -Headers (Get-AXVCtiHeaders)
}

function Get-AXVCtiRequestEvents {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][ValidateRange(1,[long]::MaxValue)][long]$RequestId,
        [ValidateRange(0,[long]::MaxValue)][long]$After=0
    )
    Invoke-RestMethod -Method Get -Uri "$script:BaseUrl/v1/requests/$RequestId/events?after=$After" -Headers (Get-AXVCtiHeaders)
}

function Stop-AXVCtiQueuedRequest {
    [CmdletBinding()]
    param([Parameter(Mandatory)][ValidateRange(1,[long]::MaxValue)][long]$RequestId)
    # DELETE only works in state 'queued'. It is not a SIP hangup operation.
    Invoke-RestMethod -Method Delete -Uri "$script:BaseUrl/v1/requests/$RequestId" -Headers (Get-AXVCtiHeaders)
}

Export-ModuleMember -Function Get-AXVCtiCapabilities, Start-AXVCtiCall, Get-AXVCtiRequest, Get-AXVCtiRequestEvents, Stop-AXVCtiQueuedRequest
